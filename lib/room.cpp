/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "room.h"

#include "csapi/kicking.h"
#include "csapi/inviting.h"
#include "csapi/banning.h"
#include "csapi/leaving.h"
#include "csapi/receipts.h"
#include "csapi/redaction.h"
#include "csapi/account-data.h"
#include "csapi/room_state.h"
#include "csapi/room_send.h"
#include "csapi/tags.h"
#include "events/simplestateevents.h"
#include "events/roomavatarevent.h"
#include "events/roommemberevent.h"
#include "events/typingevent.h"
#include "events/receiptevent.h"
#include "events/callinviteevent.h"
#include "events/callcandidatesevent.h"
#include "events/callanswerevent.h"
#include "events/callhangupevent.h"
#include "events/redactionevent.h"
#include "jobs/mediathumbnailjob.h"
#include "jobs/downloadfilejob.h"
#include "jobs/postreadmarkersjob.h"
#include "avatar.h"
#include "connection.h"
#include "user.h"
#include "converters.h"
#include "syncdata.h"

#include <QtCore/QHash>
#include <QtCore/QStringBuilder> // for efficient string concats (operator%)
#include <QtCore/QPointer>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>

#include <array>
#include <functional>
#include <cmath>

using namespace QMatrixClient;
using namespace std::placeholders;
using std::move;
#if !(defined __GLIBCXX__ && __GLIBCXX__ <= 20150123)
using std::llround;
#endif

enum EventsPlacement : int { Older = -1, Newer = 1 };

// A workaround for MSVC 2015 that fails with "error C2440: 'return':
// cannot convert from 'initializer list' to 'QMatrixClient::FileTransferInfo'"
#if (defined(_MSC_VER) && _MSC_VER < 1910) || (defined(__GNUC__) && __GNUC__ <= 4)
#  define WORKAROUND_EXTENDED_INITIALIZER_LIST
#endif

class Room::Private
{
    public:
        /** Map of user names to users. User names potentially duplicate, hence a multi-hashmap. */
        using members_map_t = QMultiHash<QString, User*>;

        Private(Connection* c, QString id_, JoinState initialJoinState)
            : q(nullptr), connection(c), id(move(id_))
            , joinState(initialJoinState)
        { }

        Room* q;

        // This updates the room displayname field (which is the way a room
        // should be shown in the room list) It should be called whenever the
        // list of members or the room name (m.room.name) or canonical alias change.
        void updateDisplayname();

        Connection* connection;
        QString id;
        JoinState joinState;
        /// The state of the room at timeline position before-0
        /// \sa timelineBase
        std::unordered_map<StateEventKey, StateEventPtr> baseState;
        /// The state of the room at timeline position after-maxTimelineIndex()
        /// \sa Room::syncEdge
        QHash<StateEventKey, const StateEventBase*> currentState;
        Timeline timeline;
        PendingEvents unsyncedEvents;
        QHash<QString, TimelineItem::index_t> eventsIndex;
        QString displayname;
        Avatar avatar;
        int highlightCount = 0;
        int notificationCount = 0;
        members_map_t membersMap;
        QList<User*> usersTyping;
        QMultiHash<QString, User*> eventIdReadUsers;
        QList<User*> membersLeft;
        int unreadMessages = 0;
        bool displayed = false;
        QString firstDisplayedEventId;
        QString lastDisplayedEventId;
        QHash<const User*, QString> lastReadEventIds;
        QString serverReadMarker;
        TagsMap tags;
        std::unordered_map<QString, EventPtr> accountData;
        QString prevBatch;
        QPointer<GetRoomEventsJob> eventsHistoryJob;

        struct FileTransferPrivateInfo
        {
#ifdef WORKAROUND_EXTENDED_INITIALIZER_LIST
            FileTransferPrivateInfo() = default;
            FileTransferPrivateInfo(BaseJob* j, QString fileName)
                : job(j), localFileInfo(fileName)
            { }
#endif
            QPointer<BaseJob> job = nullptr;
            QFileInfo localFileInfo { };
            FileTransferInfo::Status status = FileTransferInfo::Started;
            qint64 progress = 0;
            qint64 total = -1;

            void update(qint64 p, qint64 t)
            {
                if (t == 0)
                {
                    t = -1;
                    if (p == 0)
                        p = -1;
                }
                if (p != -1)
                    qCDebug(PROFILER) << "Transfer progress:" << p << "/" << t
                        << "=" << llround(double(p) / t * 100) << "%";
                progress = p; total = t;
            }
        };
        void failedTransfer(const QString& tid, const QString& errorMessage = {})
        {
            qCWarning(MAIN) << "File transfer failed for id" << tid;
            if (!errorMessage.isEmpty())
                qCWarning(MAIN) << "Message:" << errorMessage;
            fileTransfers[tid].status = FileTransferInfo::Failed;
            emit q->fileTransferFailed(tid, errorMessage);
        }
        /// A map from event/txn ids to information about the long operation;
        /// used for both download and upload operations
        QHash<QString, FileTransferPrivateInfo> fileTransfers;

        const RoomMessageEvent* getEventWithFile(const QString& eventId) const;
        QString fileNameToDownload(const RoomMessageEvent* event) const;

        //void inviteUser(User* u); // We might get it at some point in time.
        void insertMemberIntoMap(User* u);
        void renameMember(User* u, QString oldName);
        void removeMemberFromMap(const QString& username, User* u);

        /// A point in the timeline corresponding to baseState
        rev_iter_t timelineBase() const { return q->findInTimeline(-1); }

        void getPreviousContent(int limit = 10);

        template <typename EventT>
        const EventT* getCurrentState(QString stateKey = {}) const
        {
            static const EventT empty;
            const auto* evt =
                currentState.value({EventT::matrixTypeId(), stateKey}, &empty);
            Q_ASSERT(evt->type() == EventT::typeId() &&
                     evt->matrixType() == EventT::matrixTypeId());
            return static_cast<const EventT*>(evt);
        }

        bool isEventNotable(const TimelineItem& ti) const
        {
            return !ti->isRedacted() &&
                ti->senderId() != connection->userId() &&
                is<RoomMessageEvent>(*ti);
        }

        Changes addNewMessageEvents(RoomEvents&& events);
        void addHistoricalMessageEvents(RoomEvents&& events);

        /** Move events into the timeline
         *
         * Insert events into the timeline, either new or historical.
         * Pointers in the original container become empty, the ownership
         * is passed to the timeline container.
         * @param events - the range of events to be inserted
         * @param placement - position and direction of insertion: Older for
         *                    historical messages, Newer for new ones
         */
        Timeline::difference_type moveEventsToTimeline(RoomEventsRange events,
                                                       EventsPlacement placement);

        /**
         * Remove events from the passed container that are already in the timeline
         */
        void dropDuplicateEvents(RoomEvents& events) const;

        void setLastReadEvent(User* u, QString eventId);
        void updateUnreadCount(rev_iter_t from, rev_iter_t to);
        void promoteReadMarker(User* u, rev_iter_t newMarker,
                                          bool force = false);

        void markMessagesAsRead(rev_iter_t upToMarker);

        QString sendEvent(RoomEventPtr&& event);

        template <typename EventT, typename... ArgTs>
        QString sendEvent(ArgTs&&... eventArgs)
        {
            return sendEvent(makeEvent<EventT>(std::forward<ArgTs>(eventArgs)...));
        }

        QString doSendEvent(const RoomEvent* pEvent);
        PendingEvents::iterator findAsPending(const RoomEvent* rawEvtPtr);
        void onEventSendingFailure(const RoomEvent* pEvent,
                const QString& txnId, BaseJob* call = nullptr);

        template <typename EvT>
        auto requestSetState(const QString& stateKey, const EvT& event)
        {
            // TODO: Queue up state events sending (see #133).
            return connection->callApi<SetRoomStateWithKeyJob>(
                        id, EvT::matrixTypeId(), stateKey, event.contentJson());
        }

        template <typename EvT>
        auto requestSetState(const EvT& event)
        {
            return connection->callApi<SetRoomStateJob>(
                        id, EvT::matrixTypeId(), event.contentJson());
        }

        /**
         * @brief Apply redaction to the timeline
         *
         * Tries to find an event in the timeline and redact it; deletes the
         * redaction event whether the redacted event was found or not.
         */
        bool processRedaction(const RedactionEvent& redaction);

        void setTags(TagsMap newTags);

        QJsonObject toJson() const;

    private:
        QString calculateDisplayname() const;
        QString roomNameFromMemberNames(const QList<User*>& userlist) const;

        bool isLocalUser(const User* u) const
        {
            return u == q->localUser();
        }
};

Room::Room(Connection* connection, QString id, JoinState initialJoinState)
    : QObject(connection), d(new Private(connection, id, initialJoinState))
{
    setObjectName(id);
    // See "Accessing the Public Class" section in
    // https://marcmutz.wordpress.com/translated-articles/pimp-my-pimpl-%E2%80%94-reloaded/
    d->q = this;
    connect(this, &Room::userAdded, this, &Room::memberListChanged);
    connect(this, &Room::userRemoved, this, &Room::memberListChanged);
    connect(this, &Room::memberRenamed, this, &Room::memberListChanged);
    qCDebug(MAIN) << "New" << toCString(initialJoinState) << "Room:" << id;
}

Room::~Room()
{
    delete d;
}

const QString& Room::id() const
{
    return d->id;
}

const Room::Timeline& Room::messageEvents() const
{
    return d->timeline;
}

const Room::PendingEvents& Room::pendingEvents() const
{
    return d->unsyncedEvents;
}

QString Room::name() const
{
    return d->getCurrentState<RoomNameEvent>()->name();
}

QStringList Room::aliases() const
{
    return d->getCurrentState<RoomAliasesEvent>()->aliases();
}

QString Room::canonicalAlias() const
{
    return d->getCurrentState<RoomCanonicalAliasEvent>()->alias();
}

QString Room::displayName() const
{
    return d->displayname;
}

QString Room::topic() const
{
    return d->getCurrentState<RoomTopicEvent>()->topic();
}

QString Room::avatarMediaId() const
{
    return d->avatar.mediaId();
}

QUrl Room::avatarUrl() const
{
    return d->avatar.url();
}

QImage Room::avatar(int dimension)
{
    return avatar(dimension, dimension);
}

QImage Room::avatar(int width, int height)
{
    if (!d->avatar.url().isEmpty())
        return d->avatar.get(connection(), width, height,
                             [=] { emit avatarChanged(); });

    // Use the first (excluding self) user's avatar for direct chats
    const auto dcUsers = directChatUsers();
    for (auto* u: dcUsers)
        if (u != localUser())
            return u->avatar(width, height, this, [=] { emit avatarChanged(); });

    return {};
}

User* Room::user(const QString& userId) const
{
    return connection()->user(userId);
}

JoinState Room::memberJoinState(User* user) const
{
    return
        d->membersMap.contains(user->name(this), user) ? JoinState::Join :
        JoinState::Leave;
}

JoinState Room::joinState() const
{
    return d->joinState;
}

void Room::setJoinState(JoinState state)
{
    JoinState oldState = d->joinState;
    if( state == oldState )
        return;
    d->joinState = state;
    qCDebug(MAIN) << "Room" << id() << "changed state: "
                  << int(oldState) << "->" << int(state);
    emit joinStateChanged(oldState, state);
}

void Room::Private::setLastReadEvent(User* u, QString eventId)
{
    auto& storedId = lastReadEventIds[u];
    if (storedId == eventId)
        return;
    eventIdReadUsers.remove(storedId, u);
    eventIdReadUsers.insert(eventId, u);
    swap(storedId, eventId);
    emit q->lastReadEventChanged(u);
    emit q->readMarkerForUserMoved(u, eventId, storedId);
    if (isLocalUser(u))
    {
        if (storedId != serverReadMarker)
            connection->callApi<PostReadMarkersJob>(id, storedId);
        emit q->readMarkerMoved(eventId, storedId);
    }
}

void Room::Private::updateUnreadCount(rev_iter_t from, rev_iter_t to)
{
    Q_ASSERT(from >= timeline.crbegin() && from <= timeline.crend());
    Q_ASSERT(to >= from && to <= timeline.crend());

    // Catch a special case when the last read event id refers to an event
    // that has just arrived. In this case we should recalculate
    // unreadMessages and might need to promote the read marker further
    // over local-origin messages.
    const auto readMarker = q->readMarker();
    if (readMarker >= from && readMarker < to)
    {
        promoteReadMarker(q->localUser(), readMarker, true);
        return;
    }

    Q_ASSERT(to <= readMarker);

    QElapsedTimer et; et.start();
    const auto newUnreadMessages = count_if(from, to,
            std::bind(&Room::Private::isEventNotable, this, _1));
    if (et.nsecsElapsed() > profilerMinNsecs() / 10)
        qCDebug(PROFILER) << "Counting gained unread messages took" << et;

    if(newUnreadMessages > 0)
    {
        // See https://github.com/QMatrixClient/libqmatrixclient/wiki/unread_count
        if (unreadMessages < 0)
            unreadMessages = 0;

        unreadMessages += newUnreadMessages;
        qCDebug(MAIN) << "Room" << q->objectName() << "has gained"
            << newUnreadMessages << "unread message(s),"
            << (q->readMarker() == timeline.crend() ?
                    "in total at least" : "in total")
            << unreadMessages << "unread message(s)";
        emit q->unreadMessagesChanged(q);
    }
}

void Room::Private::promoteReadMarker(User* u, rev_iter_t newMarker, bool force)
{
    Q_ASSERT_X(u, __FUNCTION__, "User* should not be nullptr");
    Q_ASSERT(newMarker >= timeline.crbegin() && newMarker <= timeline.crend());

    const auto prevMarker = q->readMarker(u);
    if (!force && prevMarker <= newMarker) // Remember, we deal with reverse iterators
        return;

    Q_ASSERT(newMarker < timeline.crend());

    // Try to auto-promote the read marker over the user's own messages
    // (switch to direct iterators for that).
    auto eagerMarker = find_if(newMarker.base(), timeline.cend(),
          [=](const TimelineItem& ti) { return ti->senderId() != u->id(); });

    setLastReadEvent(u, (*(eagerMarker - 1))->id());
    if (isLocalUser(u))
    {
        const auto oldUnreadCount = unreadMessages;
        QElapsedTimer et; et.start();
        unreadMessages = count_if(eagerMarker, timeline.cend(),
                    std::bind(&Room::Private::isEventNotable, this, _1));
        if (et.nsecsElapsed() > profilerMinNsecs() / 10)
            qCDebug(PROFILER) << "Recounting unread messages took" << et;

        // See https://github.com/QMatrixClient/libqmatrixclient/wiki/unread_count
        if (unreadMessages == 0)
            unreadMessages = -1;

        if (force || unreadMessages != oldUnreadCount)
        {
            if (unreadMessages == -1)
            {
                qCDebug(MAIN) << "Room" << displayname
                              << "has no more unread messages";
            } else
                qCDebug(MAIN) << "Room" << displayname << "still has"
                              << unreadMessages << "unread message(s)";
            emit q->unreadMessagesChanged(q);
        }
    }
}

void Room::Private::markMessagesAsRead(rev_iter_t upToMarker)
{
    const auto prevMarker = q->readMarker();
    promoteReadMarker(q->localUser(), upToMarker);
    if (prevMarker != upToMarker)
        qCDebug(MAIN) << "Marked messages as read until" << *q->readMarker();

    // We shouldn't send read receipts for the local user's own messages - so
    // search earlier messages for the latest message not from the local user
    // until the previous last-read message, whichever comes first.
    for (; upToMarker < prevMarker; ++upToMarker)
    {
        if ((*upToMarker)->senderId() != q->localUser()->id())
        {
            connection->callApi<PostReceiptJob>(id, "m.read",
                                                (*upToMarker)->id());
            break;
        }
    }
}

void Room::markMessagesAsRead(QString uptoEventId)
{
    d->markMessagesAsRead(findInTimeline(uptoEventId));
}

void Room::markAllMessagesAsRead()
{
    if (!d->timeline.empty())
        d->markMessagesAsRead(d->timeline.crbegin());
}

bool Room::hasUnreadMessages() const
{
    return unreadCount() >= 0;
}

int Room::unreadCount() const
{
    return d->unreadMessages;
}

Room::rev_iter_t Room::historyEdge() const
{
    return d->timeline.crend();
}

Room::Timeline::const_iterator Room::syncEdge() const
{
    return d->timeline.cend();
}

Room::rev_iter_t Room::timelineEdge() const
{
    return historyEdge();
}

TimelineItem::index_t Room::minTimelineIndex() const
{
    return d->timeline.empty() ? 0 : d->timeline.front().index();
}

TimelineItem::index_t Room::maxTimelineIndex() const
{
    return d->timeline.empty() ? 0 : d->timeline.back().index();
}

bool Room::isValidIndex(TimelineItem::index_t timelineIndex) const
{
    return !d->timeline.empty() &&
           timelineIndex >= minTimelineIndex() &&
           timelineIndex <= maxTimelineIndex();
}

Room::rev_iter_t Room::findInTimeline(TimelineItem::index_t index) const
{
    return timelineEdge() -
            (isValidIndex(index) ? index - minTimelineIndex() + 1 : 0);
}

Room::rev_iter_t Room::findInTimeline(const QString& evtId) const
{
    if (!d->timeline.empty() && d->eventsIndex.contains(evtId))
    {
        auto it = findInTimeline(d->eventsIndex.value(evtId));
        Q_ASSERT((*it)->id() == evtId);
        return it;
    }
    return timelineEdge();
}

bool Room::displayed() const
{
    return d->displayed;
}

void Room::setDisplayed(bool displayed)
{
    if (d->displayed == displayed)
        return;

    d->displayed = displayed;
    emit displayedChanged(displayed);
    if( displayed )
    {
        resetHighlightCount();
        resetNotificationCount();
    }
}

QString Room::firstDisplayedEventId() const
{
    return d->firstDisplayedEventId;
}

Room::rev_iter_t Room::firstDisplayedMarker() const
{
    return findInTimeline(firstDisplayedEventId());
}

void Room::setFirstDisplayedEventId(const QString& eventId)
{
    if (d->firstDisplayedEventId == eventId)
        return;

    d->firstDisplayedEventId = eventId;
    emit firstDisplayedEventChanged();
}

void Room::setFirstDisplayedEvent(TimelineItem::index_t index)
{
    Q_ASSERT(isValidIndex(index));
    setFirstDisplayedEventId(findInTimeline(index)->event()->id());
}

QString Room::lastDisplayedEventId() const
{
    return d->lastDisplayedEventId;
}

Room::rev_iter_t Room::lastDisplayedMarker() const
{
    return findInTimeline(lastDisplayedEventId());
}

void Room::setLastDisplayedEventId(const QString& eventId)
{
    if (d->lastDisplayedEventId == eventId)
        return;

    d->lastDisplayedEventId = eventId;
    emit lastDisplayedEventChanged();
}

void Room::setLastDisplayedEvent(TimelineItem::index_t index)
{
    Q_ASSERT(isValidIndex(index));
    setLastDisplayedEventId(findInTimeline(index)->event()->id());
}

Room::rev_iter_t Room::readMarker(const User* user) const
{
    Q_ASSERT(user);
    return findInTimeline(d->lastReadEventIds.value(user));
}

Room::rev_iter_t Room::readMarker() const
{
    return readMarker(localUser());
}

QString Room::readMarkerEventId() const
{
    return d->lastReadEventIds.value(localUser());
}

QList<User*> Room::usersAtEventId(const QString& eventId) {
    return d->eventIdReadUsers.values(eventId);
}

int Room::notificationCount() const
{
    return d->notificationCount;
}

void Room::resetNotificationCount()
{
    if( d->notificationCount == 0 )
        return;
    d->notificationCount = 0;
    emit notificationCountChanged(this);
}

int Room::highlightCount() const
{
    return d->highlightCount;
}

void Room::resetHighlightCount()
{
    if( d->highlightCount == 0 )
        return;
    d->highlightCount = 0;
    emit highlightCountChanged(this);
}

bool Room::hasAccountData(const QString& type) const
{
    return d->accountData.find(type) != d->accountData.end();
}

const EventPtr& Room::accountData(const QString& type) const
{
    static EventPtr NoEventPtr {};
    const auto it = d->accountData.find(type);
    return it != d->accountData.end() ? it->second : NoEventPtr;
}

QStringList Room::tagNames() const
{
    return d->tags.keys();
}

TagsMap Room::tags() const
{
    return d->tags;
}

TagRecord Room::tag(const QString& name) const
{
    return d->tags.value(name);
}

std::pair<bool, QString> validatedTag(QString name)
{
    if (name.contains('.'))
        return { false, name };

    qWarning(MAIN) << "The tag" << name
                   << "doesn't follow the CS API conventions";
    name.prepend("u.");
    qWarning(MAIN) << "Using " << name << "instead";

    return { true, name };
}

void Room::addTag(const QString& name, const TagRecord& record)
{
    const auto& checkRes = validatedTag(name);
    if (d->tags.contains(name) ||
            (checkRes.first && d->tags.contains(checkRes.second)))
        return;

    emit tagsAboutToChange();
    d->tags.insert(checkRes.second, record);
    emit tagsChanged();
    connection()->callApi<SetRoomTagJob>(localUser()->id(), id(),
                                         checkRes.second, record.order);
}

void Room::addTag(const QString& name, float order)
{
    addTag(name, TagRecord{order});
}

void Room::removeTag(const QString& name)
{
    if (d->tags.contains(name))
    {
        emit tagsAboutToChange();
        d->tags.remove(name);
        emit tagsChanged();
        connection()->callApi<DeleteRoomTagJob>(localUser()->id(), id(), name);
    } else if (!name.startsWith("u."))
        removeTag("u." + name);
    else
        qWarning(MAIN) << "Tag" << name << "on room" << objectName()
                       << "not found, nothing to remove";
}

void Room::setTags(TagsMap newTags)
{
    d->setTags(move(newTags));
    connection()->callApi<SetAccountDataPerRoomJob>(
            localUser()->id(), id(), TagEvent::matrixTypeId(),
                TagEvent(d->tags).contentJson());
}

void Room::Private::setTags(TagsMap newTags)
{
    emit q->tagsAboutToChange();
    const auto keys = newTags.keys();
    for (const auto& k: keys)
    {
        const auto& checkRes = validatedTag(k);
        if (checkRes.first)
        {
            if (newTags.contains(checkRes.second))
                newTags.remove(k);
            else
                newTags.insert(checkRes.second, newTags.take(k));
        }
    }
    tags = move(newTags);
    qCDebug(MAIN) << "Room" << q->objectName() << "is tagged with"
                  << q->tagNames().join(", ");
    emit q->tagsChanged();
}

bool Room::isFavourite() const
{
    return d->tags.contains(FavouriteTag);
}

bool Room::isLowPriority() const
{
    return d->tags.contains(LowPriorityTag);
}

bool Room::isDirectChat() const
{
    return connection()->isDirectChat(id());
}

QList<User*> Room::directChatUsers() const
{
    return connection()->directChatUsers(this);
}

const RoomMessageEvent*
Room::Private::getEventWithFile(const QString& eventId) const
{
    auto evtIt = q->findInTimeline(eventId);
    if (evtIt != timeline.rend() && is<RoomMessageEvent>(**evtIt))
    {
        auto* event = evtIt->viewAs<RoomMessageEvent>();
        if (event->hasFileContent())
            return event;
    }
    qWarning() << "No files to download in event" << eventId;
    return nullptr;
}

QString Room::Private::fileNameToDownload(const RoomMessageEvent* event) const
{
    Q_ASSERT(event->hasFileContent());
    const auto* fileInfo = event->content()->fileInfo();
    QString fileName;
    if (!fileInfo->originalName.isEmpty())
    {
        fileName = QFileInfo(fileInfo->originalName).fileName();
    }
    else if (!event->plainBody().isEmpty())
    {
        // Having no better options, assume that the body has
        // the original file URL or at least the file name.
        QUrl u { event->plainBody() };
        if (u.isValid())
            fileName = QFileInfo(u.path()).fileName();
    }
    // Check the file name for sanity
    if (fileName.isEmpty() || !QTemporaryFile(fileName).open())
        return "file." % fileInfo->mimeType.preferredSuffix();

    if (QSysInfo::productType() == "windows")
    {
        const auto& suffixes = fileInfo->mimeType.suffixes();
        if (!suffixes.isEmpty() &&
                std::none_of(suffixes.begin(), suffixes.end(),
                    [&fileName] (const QString& s) {
                        return fileName.endsWith(s); }))
            return fileName % '.' % fileInfo->mimeType.preferredSuffix();
    }
    return fileName;
}

QUrl Room::urlToThumbnail(const QString& eventId)
{
    if (auto* event = d->getEventWithFile(eventId))
        if (event->hasThumbnail())
        {
            auto* thumbnail = event->content()->thumbnailInfo();
            Q_ASSERT(thumbnail != nullptr);
            return MediaThumbnailJob::makeRequestUrl(connection()->homeserver(),
                    thumbnail->url, thumbnail->imageSize);
        }
    qDebug() << "Event" << eventId << "has no thumbnail";
    return {};
}

QUrl Room::urlToDownload(const QString& eventId)
{
    if (auto* event = d->getEventWithFile(eventId))
    {
        auto* fileInfo = event->content()->fileInfo();
        Q_ASSERT(fileInfo != nullptr);
        return DownloadFileJob::makeRequestUrl(connection()->homeserver(),
                                               fileInfo->url);
    }
    return {};
}

QString Room::fileNameToDownload(const QString& eventId)
{
    if (auto* event = d->getEventWithFile(eventId))
        return d->fileNameToDownload(event);
    return {};
}

FileTransferInfo Room::fileTransferInfo(const QString& id) const
{
    auto infoIt = d->fileTransfers.find(id);
    if (infoIt == d->fileTransfers.end())
        return {};

    // FIXME: Add lib tests to make sure FileTransferInfo::status stays
    // consistent with FileTransferInfo::job

    qint64 progress = infoIt->progress;
    qint64 total = infoIt->total;
    if (total > INT_MAX)
    {
        // JavaScript doesn't deal with 64-bit integers; scale down if necessary
        progress = llround(double(progress) / total * INT_MAX);
        total = INT_MAX;
    }

#ifdef WORKAROUND_EXTENDED_INITIALIZER_LIST
    FileTransferInfo fti;
    fti.status = infoIt->status;
    fti.progress = int(progress);
    fti.total = int(total);
    fti.localDir = QUrl::fromLocalFile(infoIt->localFileInfo.absolutePath());
    fti.localPath = QUrl::fromLocalFile(infoIt->localFileInfo.absoluteFilePath());
    return fti;
#else
    return { infoIt->status, int(progress), int(total),
        QUrl::fromLocalFile(infoIt->localFileInfo.absolutePath()),
        QUrl::fromLocalFile(infoIt->localFileInfo.absoluteFilePath())
    };
#endif
}

QString Room::prettyPrint(const QString& plainText) const
{
    return QMatrixClient::prettyPrint(plainText);
}

QList< User* > Room::usersTyping() const
{
    return d->usersTyping;
}

QList< User* > Room::membersLeft() const
{
    return d->membersLeft;
}

QList< User* > Room::users() const
{
    return d->membersMap.values();
}

QStringList Room::memberNames() const
{
    QStringList res;
    for (auto u : qAsConst(d->membersMap))
        res.append( roomMembername(u) );

    return res;
}

int Room::memberCount() const
{
    return d->membersMap.size();
}

int Room::timelineSize() const
{
    return int(d->timeline.size());
}

bool Room::usesEncryption() const
{
    return !d->getCurrentState<EncryptionEvent>()->algorithm().isEmpty();
}

GetRoomEventsJob* Room::eventsHistoryJob() const
{
    return d->eventsHistoryJob;
}

void Room::Private::insertMemberIntoMap(User *u)
{
    const auto userName = u->name(q);
    // If there is exactly one namesake of the added user, signal member renaming
    // for that other one because the two should be disambiguated now.
    auto namesakes = membersMap.values(userName);
    if (namesakes.size() == 1)
        emit q->memberAboutToRename(namesakes.front(),
                                    namesakes.front()->fullName(q));
    membersMap.insert(userName, u);
    if (namesakes.size() == 1)
        emit q->memberRenamed(namesakes.front());
}

void Room::Private::renameMember(User* u, QString oldName)
{
    if (u->name(q) == oldName)
    {
        qCWarning(MAIN) << "Room::Private::renameMember(): the user "
                        << u->fullName(q)
                        << "is already known in the room under a new name.";
    }
    else if (membersMap.contains(oldName, u))
    {
        removeMemberFromMap(oldName, u);
        insertMemberIntoMap(u);
    }
    emit q->memberRenamed(u);
}

void Room::Private::removeMemberFromMap(const QString& username, User* u)
{
    User* namesake = nullptr;
    auto namesakes = membersMap.values(username);
    if (namesakes.size() == 2)
    {
        namesake = namesakes.front() == u ? namesakes.back() : namesakes.front();
        Q_ASSERT_X(namesake != u, __FUNCTION__, "Room members list is broken");
        emit q->memberAboutToRename(namesake, username);
    }
    membersMap.remove(username, u);
    // If there was one namesake besides the removed user, signal member renaming
    // for it because it doesn't need to be disambiguated anymore.
    // TODO: Think about left users.
    if (namesake)
        emit q->memberRenamed(namesake);
}

inline auto makeErrorStr(const Event& e, QByteArray msg)
{
    return msg.append("; event dump follows:\n").append(e.originalJson());
}

Room::Timeline::difference_type Room::Private::moveEventsToTimeline(
    RoomEventsRange events, EventsPlacement placement)
{
    Q_ASSERT(!events.empty());
    // Historical messages arrive in newest-to-oldest order, so the process for
    // them is almost symmetric to the one for new messages. New messages get
    // appended from index 0; old messages go backwards from index -1.
    auto index = timeline.empty() ? -((placement+1)/2) /* 1 -> -1; -1 -> 0 */ :
                 placement == Older ? timeline.front().index() :
                 timeline.back().index();
    auto baseIndex = index;
    for (auto&& e: events)
    {
        const auto eId = e->id();
        Q_ASSERT_X(e, __FUNCTION__, "Attempt to add nullptr to timeline");
        Q_ASSERT_X(!eId.isEmpty(), __FUNCTION__,
                   makeErrorStr(*e,
                    "Event with empty id cannot be in the timeline"));
        Q_ASSERT_X(!eventsIndex.contains(eId), __FUNCTION__,
                   makeErrorStr(*e, "Event is already in the timeline; "
                       "incoming events were not properly deduplicated"));
        if (placement == Older)
            timeline.emplace_front(move(e), --index);
        else
            timeline.emplace_back(move(e), ++index);
        eventsIndex.insert(eId, index);
        Q_ASSERT(q->findInTimeline(eId)->event()->id() == eId);
    }
    const auto insertedSize = (index - baseIndex) * placement;
    Q_ASSERT(insertedSize == int(events.size()));
    return insertedSize;
}

QString Room::roomMembername(const User* u) const
{
    // See the CS spec, section 11.2.2.3

    const auto username = u->name(this);
    if (username.isEmpty())
        return u->id();

    auto namesakesIt = qAsConst(d->membersMap).find(username);

    // We expect a user to be a member of the room - but technically it is
    // possible to invoke roomMemberName() even for non-members. In such case
    // we return the full name, just in case.
    if (namesakesIt == d->membersMap.cend())
        return u->fullName(this);

    auto nextUserIt = namesakesIt + 1;
    if (nextUserIt == d->membersMap.cend() || nextUserIt.key() != username)
        return username; // No disambiguation necessary

    // Check if we can get away just attaching the bridge postfix
    // (extension to the spec)
    QVector<QString> bridges;
    for (; namesakesIt != d->membersMap.cend() && namesakesIt.key() == username;
         ++namesakesIt)
    {
        const auto bridgeName = (*namesakesIt)->bridged();
        if (bridges.contains(bridgeName)) // Two accounts on the same bridge
            return u->fullName(this); // Disambiguate fully
        // Don't bother sorting, not so many bridges out there
        bridges.push_back(bridgeName);
    }

    return u->rawName(this); // Disambiguate using the bridge postfix only
}

QString Room::roomMembername(const QString& userId) const
{
    return roomMembername(user(userId));
}

void Room::updateData(SyncRoomData&& data)
{
    if( d->prevBatch.isEmpty() )
        d->prevBatch = data.timelinePrevBatch;
    setJoinState(data.joinState);

    QElapsedTimer et; et.start();
    for (auto&& event: data.accountData)
        processAccountDataEvent(move(event));

    Changes roomChanges = Change::NoChange;
    if (!data.state.empty())
    {
        et.restart();
        for (auto&& eptr: data.state)
        {
            const auto& evt = *eptr;
            Q_ASSERT(evt.isStateEvent());
            d->baseState[{evt.matrixType(),evt.stateKey()}] = move(eptr);
            roomChanges |= processStateEvent(evt);
        }

        if (data.state.size() > 9 || et.nsecsElapsed() >= profilerMinNsecs())
            qCDebug(PROFILER) << "*** Room::processStateEvents():"
                              << data.state.size() << "event(s)," << et;
    }
    if (!data.timeline.empty())
    {
        et.restart();
        roomChanges |= d->addNewMessageEvents(move(data.timeline));
        if (data.timeline.size() > 9 || et.nsecsElapsed() >= profilerMinNsecs())
            qCDebug(PROFILER) << "*** Room::addNewMessageEvents():"
                              << data.timeline.size() << "event(s)," << et;
    }
    if (roomChanges&TopicChange)
        emit topicChanged();

    if (roomChanges&NameChange)
        emit namesChanged(this);

    d->updateDisplayname();

    for( auto&& ephemeralEvent: data.ephemeral )
        processEphemeralEvent(move(ephemeralEvent));

    // See https://github.com/QMatrixClient/libqmatrixclient/wiki/unread_count
    if (data.unreadCount != -2 && data.unreadCount != d->unreadMessages)
    {
        qCDebug(MAIN) << "Setting unread_count to" << data.unreadCount;
        d->unreadMessages = data.unreadCount;
        emit unreadMessagesChanged(this);
    }

    if( data.highlightCount != d->highlightCount )
    {
        d->highlightCount = data.highlightCount;
        emit highlightCountChanged(this);
    }
    if( data.notificationCount != d->notificationCount )
    {
        d->notificationCount = data.notificationCount;
        emit notificationCountChanged(this);
    }
    if (roomChanges != Change::NoChange)
    {
        emit changed(roomChanges);
        connection()->saveRoomState(this);
    }
}

QString Room::Private::sendEvent(RoomEventPtr&& event)
{
    if (event->transactionId().isEmpty())
        event->setTransactionId(connection->generateTxnId());
    auto* pEvent = rawPtr(event);
    emit q->pendingEventAboutToAdd();
    unsyncedEvents.emplace_back(move(event));
    emit q->pendingEventAdded();
    return doSendEvent(pEvent);
}

QString Room::Private::doSendEvent(const RoomEvent* pEvent)
{
    auto txnId = pEvent->transactionId();
    // TODO, #133: Enqueue the job rather than immediately trigger it.
    if (auto call = connection->callApi<SendMessageJob>(BackgroundRequest,
                        id, pEvent->matrixType(), txnId, pEvent->contentJson()))
    {
        Room::connect(call, &BaseJob::started, q,
            [this,pEvent,txnId] {
                auto it = findAsPending(pEvent);
                if (it == unsyncedEvents.end())
                {
                    qWarning(EVENTS) << "Pending event for transaction" << txnId
                                     << "not found - got synced so soon?";
                    return;
                }
                it->setDeparted();
                emit q->pendingEventChanged(it - unsyncedEvents.begin());
            });
        Room::connect(call, &BaseJob::failure, q,
            std::bind(&Room::Private::onEventSendingFailure,
                      this, pEvent, txnId, call));
        Room::connect(call, &BaseJob::success, q,
            [this,call,pEvent,txnId] {
                // Find an event by the pointer saved in the lambda (the pointer
                // may be dangling by now but we can still search by it).
                auto it = findAsPending(pEvent);
                if (it == unsyncedEvents.end())
                {
                    qDebug(EVENTS) << "Pending event for transaction" << txnId
                                   << "already merged";
                    return;
                }

                it->setReachedServer(call->eventId());
                emit q->pendingEventChanged(it - unsyncedEvents.begin());
            });
    } else
        onEventSendingFailure(pEvent, txnId);
    return txnId;
}

Room::PendingEvents::iterator Room::Private::findAsPending(
        const RoomEvent* rawEvtPtr)
{
    const auto comp =
        [rawEvtPtr] (const auto& pe) { return pe.event() == rawEvtPtr; };

    return std::find_if(unsyncedEvents.begin(), unsyncedEvents.end(), comp);
}

void Room::Private::onEventSendingFailure(const RoomEvent* pEvent,
        const QString& txnId, BaseJob* call)
{
    auto it = findAsPending(pEvent);
    if (it == unsyncedEvents.end())
    {
        qCritical(EVENTS) << "Pending event for transaction" << txnId
                          << "could not be sent";
        return;
    }
    it->setSendingFailed(call
        ? call->statusCaption() % ": " % call->errorString()
        : tr("The call could not be started"));
    emit q->pendingEventChanged(it - unsyncedEvents.begin());
}

QString Room::retryMessage(const QString& txnId)
{
    auto it = std::find_if(d->unsyncedEvents.begin(), d->unsyncedEvents.end(),
            [txnId] (const auto& evt) { return evt->transactionId() == txnId; });
    Q_ASSERT(it != d->unsyncedEvents.end());
    qDebug(EVENTS) << "Retrying transaction" << txnId;
    it->resetStatus();
    return d->doSendEvent(it->event());
}

void Room::discardMessage(const QString& txnId)
{
    auto it = std::find_if(d->unsyncedEvents.begin(), d->unsyncedEvents.end(),
            [txnId] (const auto& evt) { return evt->transactionId() == txnId; });
    Q_ASSERT(it != d->unsyncedEvents.end());
    qDebug(EVENTS) << "Discarding transaction" << txnId;
    emit pendingEventAboutToDiscard(it - d->unsyncedEvents.begin());
    d->unsyncedEvents.erase(it);
    emit pendingEventDiscarded();
}

QString Room::postMessage(const QString& plainText, MessageEventType type)
{
    return d->sendEvent<RoomMessageEvent>(plainText, type);
}

QString Room::postPlainText(const QString& plainText)
{
    return postMessage(plainText, MessageEventType::Text);
}

QString Room::postHtmlMessage(const QString& plainText, const QString& html,
                           MessageEventType type)
{
    return d->sendEvent<RoomMessageEvent>(plainText, type,
          new EventContent::TextContent(html, QStringLiteral("text/html")));
}

QString Room::postHtmlText(const QString& plainText, const QString& html)
{
    return postHtmlMessage(plainText, html, MessageEventType::Text);
}

QString Room::postEvent(RoomEvent* event)
{
    if (usesEncryption())
    {
        qCCritical(MAIN) << "Room" << displayName()
            << "enforces encryption; sending encrypted messages is not supported yet";
    }
    return d->sendEvent(RoomEventPtr(event));
}

QString Room::postJson(const QString& matrixType,
                       const QJsonObject& eventContent)
{
    return d->sendEvent(loadEvent<RoomEvent>(basicEventJson(matrixType, eventContent)));
}

void Room::setName(const QString& newName)
{
    d->requestSetState(RoomNameEvent(newName));
}

void Room::setCanonicalAlias(const QString& newAlias)
{
    d->requestSetState(RoomCanonicalAliasEvent(newAlias));
}

void Room::setTopic(const QString& newTopic)
{
    d->requestSetState(RoomTopicEvent(newTopic));
}

bool isEchoEvent(const RoomEventPtr& le, const PendingEventItem& re)
{
    if (le->type() != re->type())
        return false;

    if (!re->id().isEmpty())
        return le->id() == re->id();
    if (!re->transactionId().isEmpty())
        return le->transactionId() == re->transactionId();

    // This one is not reliable (there can be two unsynced
    // events with the same type, sender and state key) but
    // it's the best we have for state events.
    if (re->isStateEvent())
        return le->stateKey() == re->stateKey();

    // Empty id and no state key, hmm... (shrug)
    return le->contentJson() == re->contentJson();
}

bool Room::supportsCalls() const
{
  return d->membersMap.size() == 2;
}

void Room::inviteCall(const QString& callId, const int lifetime,
                      const QString& sdp)
{
    Q_ASSERT(supportsCalls());
    postEvent(new CallInviteEvent(callId, lifetime, sdp));
}

void Room::sendCallCandidates(const QString& callId,
                              const QJsonArray& candidates)
{
    Q_ASSERT(supportsCalls());
    postEvent(new CallCandidatesEvent(callId, candidates));
}

void Room::answerCall(const QString& callId, const int lifetime,
                      const QString& sdp)
{
    Q_ASSERT(supportsCalls());
    postEvent(new CallAnswerEvent(callId, lifetime, sdp));
}

void Room::answerCall(const QString& callId, const QString& sdp)
{
    Q_ASSERT(supportsCalls());
    postEvent(new CallAnswerEvent(callId, sdp));
}

void Room::hangupCall(const QString& callId)
{
    Q_ASSERT(supportsCalls());
    postEvent(new CallHangupEvent(callId));
}

void Room::getPreviousContent(int limit)
{
    d->getPreviousContent(limit);
}

void Room::Private::getPreviousContent(int limit)
{
    if( !isJobRunning(eventsHistoryJob) )
    {
        eventsHistoryJob =
            connection->callApi<GetRoomEventsJob>(id, prevBatch, "b", "", limit);
        emit q->eventsHistoryJobChanged();
        connect( eventsHistoryJob, &BaseJob::success, q, [=] {
            prevBatch = eventsHistoryJob->end();
            addHistoricalMessageEvents(eventsHistoryJob->chunk());
        });
        connect( eventsHistoryJob, &QObject::destroyed,
                 q, &Room::eventsHistoryJobChanged);
    }
}

void Room::inviteToRoom(const QString& memberId)
{
    connection()->callApi<InviteUserJob>(id(), memberId);
}

LeaveRoomJob* Room::leaveRoom()
{
    return connection()->callApi<LeaveRoomJob>(id());
}

SetRoomStateWithKeyJob*Room::setMemberState(const QString& memberId, const RoomMemberEvent& event) const
{
    return d->requestSetState(memberId, event);
}

void Room::kickMember(const QString& memberId, const QString& reason)
{
    connection()->callApi<KickJob>(id(), memberId, reason);
}

void Room::ban(const QString& userId, const QString& reason)
{
    connection()->callApi<BanJob>(id(), userId, reason);
}

void Room::unban(const QString& userId)
{
    connection()->callApi<UnbanJob>(id(), userId);
}

void Room::redactEvent(const QString& eventId, const QString& reason)
{
    connection()->callApi<RedactEventJob>(
        id(), eventId, connection()->generateTxnId(), reason);
}

void Room::uploadFile(const QString& id, const QUrl& localFilename,
                      const QString& overrideContentType)
{
    Q_ASSERT_X(localFilename.isLocalFile(), __FUNCTION__,
               "localFilename should point at a local file");
    auto fileName = localFilename.toLocalFile();
    auto job = connection()->uploadFile(fileName, overrideContentType);
    if (isJobRunning(job))
    {
        d->fileTransfers.insert(id, { job, fileName });
        connect(job, &BaseJob::uploadProgress, this,
                [this,id] (qint64 sent, qint64 total) {
                    d->fileTransfers[id].update(sent, total);
                    emit fileTransferProgress(id, sent, total);
                });
        connect(job, &BaseJob::success, this, [this,id,localFilename,job] {
                d->fileTransfers[id].status = FileTransferInfo::Completed;
                emit fileTransferCompleted(id, localFilename, job->contentUri());
            });
        connect(job, &BaseJob::failure, this,
                std::bind(&Private::failedTransfer, d, id, job->errorString()));
        emit newFileTransfer(id, localFilename);
    } else
        d->failedTransfer(id);
}

void Room::downloadFile(const QString& eventId, const QUrl& localFilename)
{
    auto ongoingTransfer = d->fileTransfers.find(eventId);
    if (ongoingTransfer != d->fileTransfers.end() &&
            ongoingTransfer->status == FileTransferInfo::Started)
    {
        qCWarning(MAIN) << "Download for" << eventId
                        << "already started; to restart, cancel it first";
        return;
    }

    Q_ASSERT_X(localFilename.isEmpty() || localFilename.isLocalFile(),
               __FUNCTION__, "localFilename should point at a local file");
    const auto* event = d->getEventWithFile(eventId);
    if (!event)
    {
        qCCritical(MAIN)
            << eventId << "is not in the local timeline or has no file content";
        Q_ASSERT(false);
        return;
    }
    const auto fileUrl = event->content()->fileInfo()->url;
    auto filePath = localFilename.toLocalFile();
    if (filePath.isEmpty())
    {
        // Build our own file path, starting with temp directory and eventId.
        filePath = eventId;
        filePath = QDir::tempPath() % '/' % filePath.replace(':', '_') %
                '#' % d->fileNameToDownload(event);
    }
    auto job = connection()->downloadFile(fileUrl, filePath);
    if (isJobRunning(job))
    {
        // If there was a previous transfer (completed or failed), remove it.
        d->fileTransfers.remove(eventId);
        d->fileTransfers.insert(eventId, { job, job->targetFileName() });
        connect(job, &BaseJob::downloadProgress, this,
            [this,eventId] (qint64 received, qint64 total) {
                d->fileTransfers[eventId].update(received, total);
                emit fileTransferProgress(eventId, received, total);
            });
        connect(job, &BaseJob::success, this, [this,eventId,fileUrl,job] {
                d->fileTransfers[eventId].status = FileTransferInfo::Completed;
                emit fileTransferCompleted(eventId, fileUrl,
                        QUrl::fromLocalFile(job->targetFileName()));
            });
        connect(job, &BaseJob::failure, this,
                std::bind(&Private::failedTransfer, d,
                          eventId, job->errorString()));
    } else
        d->failedTransfer(eventId);
}

void Room::cancelFileTransfer(const QString& id)
{
    auto it = d->fileTransfers.find(id);
    if (it == d->fileTransfers.end())
    {
        qCWarning(MAIN) << "No information on file transfer" << id
                        << "in room" << d->id;
        return;
    }
    if (isJobRunning(it->job))
        it->job->abandon();
    d->fileTransfers.remove(id);
    emit fileTransferCancelled(id);
}

void Room::Private::dropDuplicateEvents(RoomEvents& events) const
{
    if (events.empty())
        return;

    // Multiple-remove (by different criteria), single-erase
    // 1. Check for duplicates against the timeline.
    auto dupsBegin = remove_if(events.begin(), events.end(),
            [&] (const RoomEventPtr& e)
                { return eventsIndex.contains(e->id()); });

    // 2. Check for duplicates within the batch if there are still events.
    for (auto eIt = events.begin(); distance(eIt, dupsBegin) > 1; ++eIt)
        dupsBegin = remove_if(eIt + 1, dupsBegin,
                [&] (const RoomEventPtr& e)
                    { return e->id() == (*eIt)->id(); });
    if (dupsBegin == events.end())
        return;

    qCDebug(EVENTS) << "Dropping" << distance(dupsBegin, events.end())
                    << "duplicate event(s)";
    events.erase(dupsBegin, events.end());
}

/** Make a redacted event
 *
 * This applies the redaction procedure as defined by the CS API specification
 * to the event's JSON and returns the resulting new event. It is
 * the responsibility of the caller to dispose of the original event after that.
 */
RoomEventPtr makeRedacted(const RoomEvent& target,
                          const RedactionEvent& redaction)
{
    auto originalJson = target.originalJsonObject();
    static const QStringList keepKeys =
        { EventIdKey, TypeKey, QStringLiteral("room_id"),
          QStringLiteral("sender"), QStringLiteral("state_key"),
          QStringLiteral("prev_content"), ContentKey,
          QStringLiteral("origin_server_ts") };

        std::vector<std::pair<Event::Type, QStringList>> keepContentKeysMap
        { { RoomMemberEvent::typeId(), { QStringLiteral("membership") } }
//        , { RoomCreateEvent::typeId(),    { QStringLiteral("creator") } }
//        , { RoomJoinRules::typeId(), { QStringLiteral("join_rule") } }
//        , { RoomPowerLevels::typeId(),
//            { QStringLiteral("ban"), QStringLiteral("events"),
//              QStringLiteral("events_default"), QStringLiteral("kick"),
//              QStringLiteral("redact"), QStringLiteral("state_default"),
//              QStringLiteral("users"), QStringLiteral("users_default") } }
        , { RoomAliasesEvent::typeId(),   { QStringLiteral("alias") } }
        };
    for (auto it = originalJson.begin(); it != originalJson.end();)
    {
        if (!keepKeys.contains(it.key()))
            it = originalJson.erase(it); // TODO: shred the value
        else
            ++it;
    }
    auto keepContentKeys =
            find_if(keepContentKeysMap.begin(), keepContentKeysMap.end(),
                    [&target](const auto& t) { return target.type() == t.first; } );
    if (keepContentKeys == keepContentKeysMap.end())
    {
        originalJson.remove(ContentKeyL);
        originalJson.remove(PrevContentKeyL);
    } else {
        auto content = originalJson.take(ContentKeyL).toObject();
        for (auto it = content.begin(); it != content.end(); )
        {
            if (!keepContentKeys->second.contains(it.key()))
                it = content.erase(it);
            else
                ++it;
        }
        originalJson.insert(ContentKey, content);
    }
    auto unsignedData = originalJson.take(UnsignedKeyL).toObject();
    unsignedData[RedactedCauseKeyL] = redaction.originalJsonObject();
    originalJson.insert(QStringLiteral("unsigned"), unsignedData);

    return loadEvent<RoomEvent>(originalJson);
}

bool Room::Private::processRedaction(const RedactionEvent& redaction)
{
    // Can't use findInTimeline because it returns a const iterator, and
    // we need to change the underlying TimelineItem.
    const auto pIdx = eventsIndex.find(redaction.redactedEvent());
    if (pIdx == eventsIndex.end())
        return false;

    Q_ASSERT(q->isValidIndex(*pIdx));

    auto& ti = timeline[Timeline::size_type(*pIdx - q->minTimelineIndex())];
    if (ti->isRedacted() && ti->redactedBecause()->id() == redaction.id())
    {
        qCDebug(MAIN) << "Redaction" << redaction.id()
                      << "of event" << ti->id() << "already done, skipping";
        return true;
    }

    // Make a new event from the redacted JSON and put it in the timeline
    // instead of the redacted one. oldEvent will be deleted on return.
    auto oldEvent = ti.replaceEvent(makeRedacted(*ti, redaction));
    qCDebug(MAIN) << "Redacted" << oldEvent->id() << "with" << redaction.id();
    if (oldEvent->isStateEvent())
    {
        const StateEventKey evtKey { oldEvent->matrixType(), oldEvent->stateKey() };
        Q_ASSERT(currentState.contains(evtKey));
        if (currentState[evtKey] == oldEvent.get())
        {
            Q_ASSERT(ti.index() >= 0); // Historical states can't be in currentState
            qCDebug(MAIN).nospace() << "Reverting state "
                << oldEvent->matrixType() << "/" << oldEvent->stateKey();
            // Retarget the current state to the newly made event.
            if (q->processStateEvent(*ti))
                emit q->namesChanged(q);
            updateDisplayname();
        }
    }
    q->onRedaction(*oldEvent, *ti);
    emit q->replacedEvent(ti.event(), rawPtr(oldEvent));
    return true;
}

Connection* Room::connection() const
{
    Q_ASSERT(d->connection);
    return d->connection;
}

User* Room::localUser() const
{
    return connection()->user();
}

inline bool isRedaction(const RoomEventPtr& ep)
{
    Q_ASSERT(ep);
    return is<RedactionEvent>(*ep);
}

Room::Changes Room::Private::addNewMessageEvents(RoomEvents&& events)
{
    dropDuplicateEvents(events);
    if (events.empty())
        return Change::NoChange;

    // Pre-process redactions so that events that get redacted in the same
    // batch landed in the timeline already redacted.
    // NB: We have to store redaction events to the timeline too - see #220.
    auto redactionIt = std::find_if(events.begin(), events.end(), isRedaction);
    for(const auto& eptr: RoomEventsRange(redactionIt, events.end()))
        if (auto* r = eventCast<RedactionEvent>(eptr))
        {
            // Try to find the target in the timeline, then in the batch.
            if (processRedaction(*r))
                continue;
            auto targetIt = std::find_if(events.begin(), redactionIt,
                [id=r->redactedEvent()] (const RoomEventPtr& ep) {
                    return ep->id() == id;
                });
            if (targetIt != redactionIt)
                *targetIt = makeRedacted(**targetIt, *r);
            else
                qCDebug(MAIN) << "Redaction" << r->id()
                              << "ignored: target event" << r->redactedEvent()
                              << "is not found";
            // If the target event comes later, it comes already redacted.
        }

    // State changes arrive as a part of timeline; the current room state gets
    // updated before merging events to the timeline because that's what
    // clients historically expect. This may eventually change though if we
    // postulate that the current state is only current between syncs but not
    // within a sync.
    Changes stateChanges = Change::NoChange;
    for (const auto& eptr: events)
        stateChanges |= q->processStateEvent(*eptr);

    auto timelineSize = timeline.size();
    auto totalInserted = 0;
    for (auto it = events.begin(); it != events.end();)
    {
        auto nextPendingPair = findFirstOf(it, events.end(),
                    unsyncedEvents.begin(), unsyncedEvents.end(), isEchoEvent);
        auto nextPending = nextPendingPair.first;

        if (it != nextPending)
        {
            RoomEventsRange eventsSpan { it, nextPending };
            emit q->aboutToAddNewMessages(eventsSpan);
            auto insertedSize = moveEventsToTimeline(eventsSpan, Newer);
            totalInserted += insertedSize;
            auto firstInserted = timeline.cend() - insertedSize;
            q->onAddNewTimelineEvents(firstInserted);
            emit q->addedMessages(firstInserted->index(), timeline.back().index());
        }
        if (nextPending == events.end())
            break;

        it = nextPending + 1;
        emit q->pendingEventAboutToMerge(nextPending->get(),
                    nextPendingPair.second - unsyncedEvents.begin());
        qDebug(EVENTS) << "Merging pending event from transaction"
                       << (*nextPending)->transactionId() << "into"
                       << (*nextPending)->id();
        unsyncedEvents.erase(nextPendingPair.second);
        if (auto insertedSize = moveEventsToTimeline({nextPending, it}, Newer))
        {
            totalInserted += insertedSize;
            q->onAddNewTimelineEvents(timeline.cend() - insertedSize);
        }
        emit q->pendingEventMerged();
    }
    // Events merged and transferred from `events` to `timeline` now.
    const auto from = timeline.cend() - totalInserted;

    if (q->supportsCalls())
        for (auto it = from; it != timeline.cend(); ++it)
            if (auto* evt = it->viewAs<CallEventBase>())
                emit q->callEvent(q, evt);

    if (totalInserted > 0)
    {
        qCDebug(MAIN)
                << "Room" << q->objectName() << "received" << totalInserted
                << "new events; the last event is now" << timeline.back();

        // The first event in the just-added batch (referred to by `from`)
        // defines whose read marker can possibly be promoted any further over
        // the same author's events newly arrived. Others will need explicit
        // read receipts from the server (or, for the local user,
        // markMessagesAsRead() invocation) to promote their read markers over
        // the new message events.
        auto firstWriter = q->user((*from)->senderId());
        if (q->readMarker(firstWriter) != timeline.crend())
        {
            promoteReadMarker(firstWriter, rev_iter_t(from) - 1);
            qCDebug(MAIN) << "Auto-promoted read marker for" << firstWriter->id()
                          << "to" << *q->readMarker(firstWriter);
        }

        updateUnreadCount(timeline.crbegin(), rev_iter_t(from));
    }

    Q_ASSERT(timeline.size() == timelineSize + totalInserted);
    return stateChanges;
}

void Room::Private::addHistoricalMessageEvents(RoomEvents&& events)
{
    QElapsedTimer et; et.start();
    const auto timelineSize = timeline.size();

    dropDuplicateEvents(events);
    if (events.empty())
        return;

    // In case of lazy-loading new members may be loaded with historical
    // messages. Also, the cache doesn't store events with empty content;
    // so when such events show up in the timeline they should be properly
    // incorporated.
    for (const auto& eptr: events)
    {
        const auto& e = *eptr;
        if (e.isStateEvent() &&
                !currentState.contains({e.matrixType(), e.stateKey()}))
        {
            q->processStateEvent(e);
        }
    }

    emit q->aboutToAddHistoricalMessages(events);
    const auto insertedSize = moveEventsToTimeline(events, Older);
    const auto from = timeline.crend() - insertedSize;

    qCDebug(MAIN) << "Room" << displayname << "received" << insertedSize
                  << "past events; the oldest event is now" << timeline.front();
    q->onAddHistoricalTimelineEvents(from);
    emit q->addedMessages(timeline.front().index(), from->index());

    if (from <= q->readMarker())
        updateUnreadCount(from, timeline.crend());

    Q_ASSERT(timeline.size() == timelineSize + insertedSize);
    if (insertedSize > 9 || et.nsecsElapsed() >= profilerMinNsecs())
        qCDebug(PROFILER) << "*** Room::addHistoricalMessageEvents():"
                          << insertedSize << "event(s)," << et;
}

Room::Changes Room::processStateEvent(const RoomEvent& e)
{
    if (!e.isStateEvent())
        return Change::NoChange;

    d->currentState[{e.matrixType(),e.stateKey()}] =
            static_cast<const StateEventBase*>(&e);
    if (!is<RoomMemberEvent>(e))
        qCDebug(EVENTS) << "Room state event:" << e;

    return visit(e
        , [] (const RoomNameEvent&) {
            return NameChange;
        }
        , [] (const RoomAliasesEvent&) {
            return OtherChange;
        }
        , [this] (const RoomCanonicalAliasEvent& evt) {
            setObjectName(evt.alias().isEmpty() ? d->id : evt.alias());
            return CanonicalAliasChange;
        }
        , [] (const RoomTopicEvent&) {
            return TopicChange;
        }
        , [this] (const RoomAvatarEvent& evt) {
            if (d->avatar.updateUrl(evt.url()))
                emit avatarChanged();
            return AvatarChange;
        }
        , [this] (const RoomMemberEvent& evt) {
            auto* u = user(evt.userId());
            u->processEvent(evt, this);
            if (u == localUser() && memberJoinState(u) == JoinState::Invite
                    && evt.isDirect())
                connection()->addToDirectChats(this, user(evt.senderId()));

            if( evt.membership() == MembershipType::Join )
            {
                if (memberJoinState(u) != JoinState::Join)
                {
                    d->insertMemberIntoMap(u);
                    connect(u, &User::nameAboutToChange, this,
                        [=] (QString newName, QString, const Room* context) {
                            if (context == this)
                                emit memberAboutToRename(u, newName);
                        });
                    connect(u, &User::nameChanged, this,
                        [=] (QString, QString oldName, const Room* context) {
                            if (context == this)
                                d->renameMember(u, oldName);
                        });
                    emit userAdded(u);
                }
            }
            else if( evt.membership() != MembershipType::Join )
            {
                if (memberJoinState(u) == JoinState::Join)
                {
                    if (evt.membership() == MembershipType::Invite)
                        qCWarning(MAIN) << "Invalid membership change:" << evt;
                    if (!d->membersLeft.contains(u))
                        d->membersLeft.append(u);
                    d->removeMemberFromMap(u->name(this), u);
                    emit userRemoved(u);
                }
            }
            return MembersChange;
        }
        , [this] (const EncryptionEvent&) {
            emit encryption(); // It can only be done once, so emit it here.
            return EncryptionOn;
        }
    );
}

void Room::processEphemeralEvent(EventPtr&& event)
{
    QElapsedTimer et; et.start();
    if (auto* evt = eventCast<TypingEvent>(event))
    {
        d->usersTyping.clear();
        for( const QString& userId: qAsConst(evt->users()) )
        {
            auto u = user(userId);
            if (memberJoinState(u) == JoinState::Join)
                d->usersTyping.append(u);
        }
        if (evt->users().size() > 3 || et.nsecsElapsed() >= profilerMinNsecs())
            qCDebug(PROFILER) << "*** Room::processEphemeralEvent(typing):"
                << evt->users().size() << "users," << et;
        emit typingChanged();
    }
    if (auto* evt = eventCast<ReceiptEvent>(event))
    {
        int totalReceipts = 0;
        for( const auto &p: qAsConst(evt->eventsWithReceipts()) )
        {
            totalReceipts += p.receipts.size();
            {
                if (p.receipts.size() == 1)
                    qCDebug(EPHEMERAL) << "Marking" << p.evtId
                        << "as read for" << p.receipts[0].userId;
                else
                    qCDebug(EPHEMERAL) << "Marking" << p.evtId
                        << "as read for" << p.receipts.size() << "users";
            }
            const auto newMarker = findInTimeline(p.evtId);
            if (newMarker != timelineEdge())
            {
                for( const Receipt& r: p.receipts )
                {
                    if (r.userId == connection()->userId())
                        continue; // FIXME, #185
                    auto u = user(r.userId);
                    if (memberJoinState(u) == JoinState::Join)
                        d->promoteReadMarker(u, newMarker);
                }
            } else
            {
                qCDebug(EPHEMERAL) << "Event" << p.evtId
                    << "not found; saving read receipts anyway";
                // If the event is not found (most likely, because it's too old
                // and hasn't been fetched from the server yet), but there is
                // a previous marker for a user, keep the previous marker.
                // Otherwise, blindly store the event id for this user.
                for( const Receipt& r: p.receipts )
                {
                    if (r.userId == connection()->userId())
                        continue; // FIXME, #185
                    auto u = user(r.userId);
                    if (memberJoinState(u) == JoinState::Join &&
                            readMarker(u) == timelineEdge())
                        d->setLastReadEvent(u, p.evtId);
                }
            }
        }
        if (evt->eventsWithReceipts().size() > 3 || totalReceipts > 10 ||
                et.nsecsElapsed() >= profilerMinNsecs())
            qCDebug(PROFILER) << "*** Room::processEphemeralEvent(receipts):"
                << evt->eventsWithReceipts().size()
                << "event(s) with" << totalReceipts << "receipt(s)," << et;
    }
}

void Room::processAccountDataEvent(EventPtr&& event)
{
    if (auto* evt = eventCast<TagEvent>(event))
        d->setTags(evt->tags());

    if (auto* evt = eventCast<ReadMarkerEvent>(event))
    {
        auto readEventId = evt->event_id();
        qCDebug(MAIN) << "Server-side read marker at" << readEventId;
        d->serverReadMarker = readEventId;
        const auto newMarker = findInTimeline(readEventId);
        if (newMarker != timelineEdge())
            d->markMessagesAsRead(newMarker);
        else
            d->setLastReadEvent(localUser(), readEventId);
    }
    // For all account data events
    auto& currentData = d->accountData[event->matrixType()];
    // A polymorphic event-specific comparison might be a bit more
    // efficient; maaybe do it another day
    if (!currentData || currentData->contentJson() != event->contentJson())
    {
        emit accountDataAboutToChange(event->matrixType());
        currentData = move(event);
        qCDebug(MAIN) << "Updated account data of type"
                      << currentData->matrixType();
        emit accountDataChanged(currentData->matrixType());
    }
}

QString Room::Private::roomNameFromMemberNames(const QList<User *> &userlist) const
{
    // This is part 3(i,ii,iii) in the room displayname algorithm described
    // in the CS spec (see also Room::Private::updateDisplayname() ).
    // The spec requires to sort users lexicographically by state_key (user id)
    // and use disambiguated display names of two topmost users excluding
    // the current one to render the name of the room.

    // std::array is the leanest C++ container
    std::array<User*, 2> first_two = { {nullptr, nullptr} };
    std::partial_sort_copy(
        userlist.begin(), userlist.end(),
        first_two.begin(), first_two.end(),
        [this](const User* u1, const User* u2) {
            // Filter out the "me" user so that it never hits the room name
            return isLocalUser(u2) || (!isLocalUser(u1) && u1->id() < u2->id());
        }
    );

    // Spec extension. A single person in the chat but not the local user
    // (the local user is invited).
    if (userlist.size() == 1 && !isLocalUser(first_two.front()) &&
            joinState == JoinState::Invite)
        return tr("Invitation from %1")
                .arg(q->roomMembername(first_two.front()));

    // i. One-on-one chat. first_two[1] == localUser() in this case.
    if (userlist.size() == 2)
        return q->roomMembername(first_two[0]);

    // ii. Two users besides the current one.
    if (userlist.size() == 3)
        return tr("%1 and %2")
                .arg(q->roomMembername(first_two[0]),
                     q->roomMembername(first_two[1]));

    // iii. More users.
    if (userlist.size() > 3)
        return tr("%1 and %Ln other(s)", "", userlist.size() - 3)
                .arg(q->roomMembername(first_two[0]));

    // userlist.size() < 2 - apparently, there's only current user in the room
    return QString();
}

QString Room::Private::calculateDisplayname() const
{
    // CS spec, section 11.2.2.5 Calculating the display name for a room
    // Numbers below refer to respective parts in the spec.

    // 1. Name (from m.room.name)
    auto dispName = q->name();
    if (!dispName.isEmpty()) {
        return dispName;
    }

    // 2. Canonical alias
    dispName = q->canonicalAlias();
    if (!dispName.isEmpty())
        return dispName;

    // Using m.room.aliases in naming is explicitly discouraged by the spec
    //if (!q->aliases().empty() && !q->aliases().at(0).isEmpty())
    //    return q->aliases().at(0);

    // 3. Room members
    dispName = roomNameFromMemberNames(membersMap.values());
    if (!dispName.isEmpty())
        return dispName;

    // 4. Users that previously left the room
    dispName = roomNameFromMemberNames(membersLeft);
    if (!dispName.isEmpty())
        return tr("Empty room (was: %1)").arg(dispName);

    // 5. Fail miserably
    return tr("Empty room (%1)").arg(id);
}

void Room::Private::updateDisplayname()
{
    auto swappedName = calculateDisplayname();
    if (swappedName != displayname)
    {
        emit q->displaynameAboutToChange(q);
        swap(displayname, swappedName);
        qDebug(MAIN) << q->objectName() << "has changed display name from"
                     << swappedName << "to" << displayname;
        emit q->displaynameChanged(q, swappedName);
    }
}

QJsonObject Room::Private::toJson() const
{
    QElapsedTimer et; et.start();
    QJsonObject result;
    {
        QJsonArray stateEvents;

        for (const auto* evt: currentState)
        {
            Q_ASSERT(evt->isStateEvent());
            if ((evt->isRedacted() && !is<RoomMemberEvent>(*evt)) ||
                    evt->contentJson().isEmpty())
                continue;

            auto json = evt->fullJson();
            auto unsignedJson = evt->unsignedJson();
            unsignedJson.remove(QStringLiteral("prev_content"));
            json[UnsignedKeyL] = unsignedJson;
            stateEvents.append(json);
        }

        const auto stateObjName = joinState == JoinState::Invite ?
                    QStringLiteral("invite_state") : QStringLiteral("state");
        result.insert(stateObjName,
            QJsonObject {{ QStringLiteral("events"), stateEvents }});
    }

    if (!accountData.empty())
    {
        QJsonArray accountDataEvents;
        for (const auto& e: accountData)
        {
            if (!e.second->contentJson().isEmpty())
                accountDataEvents.append(e.second->fullJson());
        }
        result.insert(QStringLiteral("account_data"),
                      QJsonObject {{ QStringLiteral("events"), accountDataEvents }});
    }

    QJsonObject unreadNotifObj
        { { SyncRoomData::UnreadCountKey, unreadMessages } };

    if (highlightCount > 0)
        unreadNotifObj.insert(QStringLiteral("highlight_count"), highlightCount);
    if (notificationCount > 0)
        unreadNotifObj.insert(QStringLiteral("notification_count"), notificationCount);

    result.insert(QStringLiteral("unread_notifications"), unreadNotifObj);

    if (et.elapsed() > 30)
        qCDebug(PROFILER) << "Room::toJson() for" << displayname << "took" << et;

    return result;
}

QJsonObject Room::toJson() const
{
    return d->toJson();
}

MemberSorter Room::memberSorter() const
{
    return MemberSorter(this);
}

bool MemberSorter::operator()(User *u1, User *u2) const
{
    return operator()(u1, room->roomMembername(u2));
}

bool MemberSorter::operator ()(User* u1, const QString& u2name) const
{
    auto n1 = room->roomMembername(u1);
    if (n1.startsWith('@'))
        n1.remove(0, 1);
    auto n2 = u2name.midRef(u2name.startsWith('@') ? 1 : 0);

    return n1.localeAwareCompare(n2) < 0;
}

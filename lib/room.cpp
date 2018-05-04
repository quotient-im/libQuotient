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

#include "jobs/generated/kicking.h"
#include "jobs/generated/inviting.h"
#include "jobs/generated/banning.h"
#include "jobs/generated/leaving.h"
#include "jobs/generated/receipts.h"
#include "jobs/generated/redaction.h"
#include "jobs/generated/account-data.h"
#include "jobs/generated/message_pagination.h"
#include "jobs/setroomstatejob.h"
#include "events/simplestateevents.h"
#include "events/roomavatarevent.h"
#include "events/roommemberevent.h"
#include "events/typingevent.h"
#include "events/receiptevent.h"
#include "events/redactionevent.h"
#include "jobs/sendeventjob.h"
#include "jobs/mediathumbnailjob.h"
#include "jobs/downloadfilejob.h"
#include "jobs/postreadmarkersjob.h"
#include "avatar.h"
#include "connection.h"
#include "user.h"

#include <QtCore/QHash>
#include <QtCore/QStringBuilder> // for efficient string concats (operator%)
#include <QtCore/QElapsedTimer>
#include <QtCore/QPointer>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QRegularExpression>

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
        typedef QMultiHash<QString, User*> members_map_t;

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
        Timeline timeline;
        QHash<QString, TimelineItem::index_t> eventsIndex;
        QString id;
        QStringList aliases;
        QString canonicalAlias;
        QString name;
        QString displayname;
        QString topic;
        QString encryptionAlgorithm;
        Avatar avatar;
        JoinState joinState;
        int highlightCount = 0;
        int notificationCount = 0;
        members_map_t membersMap;
        QList<User*> usersTyping;
        QList<User*> membersLeft;
        int unreadMessages = 0;
        bool displayed = false;
        QString firstDisplayedEventId;
        QString lastDisplayedEventId;
        QHash<const User*, QString> lastReadEventIds;
        QString serverReadMarker;
        TagsMap tags;
        QHash<QString, AccountDataMap> accountData;
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
        // A map from event/txn ids to information about the long operation;
        // used for both download and upload operations
        QHash<QString, FileTransferPrivateInfo> fileTransfers;

        const RoomMessageEvent* getEventWithFile(const QString& eventId) const;
        QString fileNameToDownload(const RoomMessageEvent* event) const;

        //void inviteUser(User* u); // We might get it at some point in time.
        void insertMemberIntoMap(User* u);
        void renameMember(User* u, QString oldName);
        void removeMemberFromMap(const QString& username, User* u);

        void getPreviousContent(int limit = 10);

        bool isEventNotable(const TimelineItem& ti) const
        {
            return !ti->isRedacted() &&
                ti->senderId() != connection->userId() &&
                ti->type() == EventType::RoomMessage;
        }

        void addNewMessageEvents(RoomEvents&& events);
        void addHistoricalMessageEvents(RoomEvents&& events);

        /**
         * @brief Move events into the timeline
         *
         * Insert events into the timeline, either new or historical.
         * Pointers in the original container become empty, the ownership
         * is passed to the timeline container.
         * @param events - the range of events to be inserted
         * @param placement - position and direction of insertion: Older for
         *                    historical messages, Newer for new ones
         */
        Timeline::size_type insertEvents(RoomEventsRange&& events,
                                         EventsPlacement placement);

        /**
         * Removes events from the passed container that are already in the timeline
         */
        void dropDuplicateEvents(RoomEvents* events) const;

        void setLastReadEvent(User* u, const QString& eventId);
        void updateUnreadCount(rev_iter_t from, rev_iter_t to);
        void promoteReadMarker(User* u, rev_iter_t newMarker,
                                          bool force = false);

        void markMessagesAsRead(rev_iter_t upToMarker);

        /**
         * @brief Apply redaction to the timeline
         *
         * Tries to find an event in the timeline and redact it; deletes the
         * redaction event whether the redacted event was found or not.
         */
        void processRedaction(event_ptr_tt<RedactionEvent>&& redaction);

        void broadcastTagUpdates()
        {
            connection->callApi<SetAccountDataPerRoomJob>(
                    connection->userId(), id, TagEvent::typeId(),
                        TagEvent(tags).toJson());
            emit q->tagsChanged();
        }

        QJsonObject toJson() const;

    private:
        QString calculateDisplayname() const;
        QString roomNameFromMemberNames(const QList<User*>& userlist) const;

        bool isLocalUser(const User* u) const
        {
            return u == q->localUser();
        }
};

RoomEventPtr TimelineItem::replaceEvent(RoomEventPtr&& other)
{
    return std::exchange(evt, move(other));
}

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

QString Room::name() const
{
    return d->name;
}

QStringList Room::aliases() const
{
    return d->aliases;
}

QString Room::canonicalAlias() const
{
    return d->canonicalAlias;
}

QString Room::displayName() const
{
    return d->displayname;
}

QString Room::topic() const
{
    return d->topic;
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

    // Use the other side's avatar for 1:1's
    if (d->membersMap.size() == 2)
    {
        auto theOtherOneIt = d->membersMap.begin();
        if (theOtherOneIt.value() == localUser())
            ++theOtherOneIt;
        return (*theOtherOneIt)->avatar(width, height, this,
                                        [=] { emit avatarChanged(); });
    }
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

void Room::Private::setLastReadEvent(User* u, const QString& eventId)
{
    auto& storedId = lastReadEventIds[u];
    if (storedId == eventId)
        return;
    storedId = eventId;
    emit q->lastReadEventChanged(u);
    if (isLocalUser(u))
    {
        if (eventId != serverReadMarker)
            connection->callApi<PostReadMarkersJob>(id, eventId);
        emit q->readMarkerMoved();
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
        qCDebug(MAIN) << "Discovered last read event in room" << displayname;
        promoteReadMarker(q->localUser(), readMarker, true);
        return;
    }

    Q_ASSERT(to <= readMarker);

    QElapsedTimer et; et.start();
    const auto newUnreadMessages = count_if(from, to,
            std::bind(&Room::Private::isEventNotable, this, _1));
    if (et.nsecsElapsed() > 10000)
        qCDebug(PROFILER) << "Counting gained unread messages took" << et;

    if(newUnreadMessages > 0)
    {
        // See https://github.com/QMatrixClient/libqmatrixclient/wiki/unread_count
        if (unreadMessages < 0)
            unreadMessages = 0;

        unreadMessages += newUnreadMessages;
        qCDebug(MAIN) << "Room" << displayname << "has gained"
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
        if (et.nsecsElapsed() > 10000)
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

Room::rev_iter_t Room::timelineEdge() const
{
    return d->timeline.crend();
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
        return findInTimeline(d->eventsIndex.value(evtId));
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
    return d->accountData.contains(type);
}

Room::AccountDataMap Room::accountData(const QString& type) const
{
    return d->accountData.value(type);
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

void Room::addTag(const QString& name, const TagRecord& record)
{
    if (d->tags.contains(name))
        return;

    d->tags.insert(name, record);
    d->broadcastTagUpdates();
}

void Room::removeTag(const QString& name)
{
    if (!d->tags.contains(name))
        return;

    d->tags.remove(name);
    d->broadcastTagUpdates();
}

void Room::setTags(const TagsMap& newTags)
{
    if (newTags == d->tags)
        return;
    d->tags = newTags;
    d->broadcastTagUpdates();
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

QList<const User*> Room::directChatUsers() const
{
    return connection()->directChatUsers(this);
}

const RoomMessageEvent*
Room::Private::getEventWithFile(const QString& eventId) const
{
    auto evtIt = q->findInTimeline(eventId);
    if (evtIt != timeline.rend() &&
            evtIt->event()->type() == EventType::RoomMessage)
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

static const auto RegExpOptions =
    QRegularExpression::CaseInsensitiveOption
    | QRegularExpression::OptimizeOnFirstUsageOption
    | QRegularExpression::UseUnicodePropertiesOption;

// regexp is originally taken from Konsole (https://github.com/KDE/konsole)
// full url:
// protocolname:// or www. followed by anything other than whitespaces,
// <, >, ' or ", and ends before whitespaces, <, >, ', ", ], !, ), :,
// comma or dot
// Note: outer parentheses are a part of C++ raw string delimiters, not of
// the regex (see http://en.cppreference.com/w/cpp/language/string_literal).
static const QRegularExpression FullUrlRegExp(QStringLiteral(
        R"(((www\.(?!\.)|[a-z][a-z0-9+.-]*://)(&(?![lg]t;)|[^&\s<>'"])+(&(?![lg]t;)|[^&!,.\s<>'"\]):])))"
    ), RegExpOptions);
// email address:
// [word chars, dots or dashes]@[word chars, dots or dashes].[word chars]
static const QRegularExpression EmailAddressRegExp(QStringLiteral(
        R"((mailto:)?(\b(\w|\.|-)+@(\w|\.|-)+\.\w+\b))"
    ), RegExpOptions);

/** Converts all that looks like a URL into HTML links */
static void linkifyUrls(QString& htmlEscapedText)
{
    // NOTE: htmlEscapedText is already HTML-escaped (no literal <,>,&)!

    htmlEscapedText.replace(EmailAddressRegExp,
                 QStringLiteral(R"(<a href="mailto:\2">\1\2</a>)"));
    htmlEscapedText.replace(FullUrlRegExp,
                 QStringLiteral(R"(<a href="\1">\1</a>)"));
}

QString Room::prettyPrint(const QString& plainText) const
{
    auto pt = QStringLiteral("<span style='white-space:pre-wrap'>") +
            plainText.toHtmlEscaped() + QStringLiteral("</span>");
    pt.replace('\n', "<br/>");

    linkifyUrls(pt);
    return pt;
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
    for (auto u : d->membersMap)
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
    return !d->encryptionAlgorithm.isEmpty();
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

Room::Timeline::size_type Room::Private::insertEvents(RoomEventsRange&& events,
                                                      EventsPlacement placement)
{
    // Historical messages arrive in newest-to-oldest order, so the process for
    // them is symmetric to the one for new messages.
    auto index = timeline.empty() ? -int(placement) :
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
    // Pointers in "events" are empty now, but events.size() didn't change
    Q_ASSERT(int(events.size()) == (index - baseIndex) * int(placement));
    return events.size();
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

    bool emitNamesChanged = false;
    if (!data.state.empty())
    {
        et.restart();
        for (const auto& e: data.state)
            emitNamesChanged |= processStateEvent(*e);

        qCDebug(PROFILER) << "*** Room::processStateEvents():"
                          << data.state.size() << "event(s)," << et;
    }
    if (!data.timeline.empty())
    {
        et.restart();
        // State changes can arrive in a timeline event; so check those.
        for (const auto& e: data.timeline)
            emitNamesChanged |= processStateEvent(*e);
        qCDebug(PROFILER) << "*** Room::processStateEvents(timeline):"
                          << data.timeline.size() << "event(s)," << et;
    }
    if (emitNamesChanged)
        emit namesChanged(this);
    d->updateDisplayname();

    if (!data.timeline.empty())
    {
        et.restart();
        d->addNewMessageEvents(move(data.timeline));
        qCDebug(PROFILER) << "*** Room::addNewMessageEvents():" << et;
    }
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
}

void Room::postMessage(const QString& type, const QString& plainText)
{
    postMessage(RoomMessageEvent { plainText, type });
}

void Room::postMessage(const QString& plainText, MessageEventType type)
{
    postMessage(RoomMessageEvent { plainText, type });
}

void Room::postMessage(const RoomMessageEvent& event)
{
    if (usesEncryption())
    {
        qCCritical(MAIN) << "Room" << displayName()
            << "enforces encryption; sending encrypted messages is not supported yet";
    }
    connection()->callApi<SendEventJob>(id(), event);
}

void Room::setName(const QString& newName)
{
    connection()->callApi<SetRoomStateJob>(id(), RoomNameEvent(newName));
}

void Room::setCanonicalAlias(const QString& newAlias)
{
    connection()->callApi<SetRoomStateJob>(id(),
                                           RoomCanonicalAliasEvent(newAlias));
}

void Room::setTopic(const QString& newTopic)
{
    RoomTopicEvent evt(newTopic);
    connection()->callApi<SetRoomStateJob>(id(), evt);
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
        connect( eventsHistoryJob, &BaseJob::success, q, [=] {
            prevBatch = eventsHistoryJob->end();
            addHistoricalMessageEvents(eventsHistoryJob->chunk());
        });
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

void Room::Private::dropDuplicateEvents(RoomEvents* events) const
{
    if (events->empty())
        return;

    // Multiple-remove (by different criteria), single-erase
    // 1. Check for duplicates against the timeline.
    auto dupsBegin = remove_if(events->begin(), events->end(),
            [&] (const RoomEventPtr& e)
                { return eventsIndex.contains(e->id()); });

    // 2. Check for duplicates within the batch if there are still events.
    for (auto eIt = events->begin(); distance(eIt, dupsBegin) > 1; ++eIt)
        dupsBegin = remove_if(eIt + 1, dupsBegin,
                [&] (const RoomEventPtr& e)
                    { return e->id() == (*eIt)->id(); });
    if (dupsBegin == events->end())
        return;

    qCDebug(EVENTS) << "Dropping" << distance(dupsBegin, events->end())
                    << "duplicate event(s)";
    events->erase(dupsBegin, events->end());
}

inline bool isRedaction(const RoomEventPtr& e)
{
    return e->type() == EventType::Redaction;
}

void Room::Private::processRedaction(event_ptr_tt<RedactionEvent>&& redaction)
{
    const auto pIdx = eventsIndex.find(redaction->redactedEvent());
    if (pIdx == eventsIndex.end())
    {
        qCDebug(MAIN) << "Redaction" << redaction->id()
                      << "ignored: target event not found";
        return; // If the target events comes later, it comes already redacted.
    }
    Q_ASSERT(q->isValidIndex(*pIdx));

    auto& ti = timeline[Timeline::size_type(*pIdx - q->minTimelineIndex())];

    // Apply the redaction procedure from chapter 6.5 of The Spec
    auto originalJson = ti->originalJsonObject();
    if (originalJson.value("unsigned").toObject()
            .value("redacted_because").toObject()
            .value("event_id") == redaction->id())
    {
        qCDebug(MAIN) << "Redaction" << redaction->id()
            << "of event" << ti.event()->id() << "already done, skipping";
        return;
    }
    static const QStringList keepKeys =
        { "event_id", "type", "room_id", "sender", "state_key",
          "prev_content", "content", "origin_server_ts" };
    static const
        std::vector<std::pair<EventType, QStringList>> keepContentKeysMap
        { { Event::Type::RoomMember,    { "membership" } }
        , { Event::Type::RoomCreate,    { "creator" } }
        , { Event::Type::RoomJoinRules, { "join_rule" } }
        , { Event::Type::RoomPowerLevels,
            { "ban", "events", "events_default", "kick", "redact",
              "state_default", "users", "users_default" } }
        , { Event::Type::RoomAliases,   { "alias" } }
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
                    [&ti](const auto& t) { return ti->type() == t.first; } );
    if (keepContentKeys == keepContentKeysMap.end())
    {
        originalJson.remove("content");
        originalJson.remove("prev_content");
    } else {
        auto content = originalJson.take("content").toObject();
        for (auto it = content.begin(); it != content.end(); )
        {
            if (!keepContentKeys->second.contains(it.key()))
                it = content.erase(it);
            else
                ++it;
        }
        originalJson.insert("content", content);
    }
    auto unsignedData = originalJson.take("unsigned").toObject();
    unsignedData["redacted_because"] = redaction->originalJsonObject();
    originalJson.insert("unsigned", unsignedData);

    // Make a new event from the redacted JSON, exchange events,
    // notify everyone and delete the old event
    RoomEventPtr oldEvent
        { ti.replaceEvent(makeEvent<RoomEvent>(originalJson)) };
    q->onRedaction(*oldEvent, *ti.event());
    qCDebug(MAIN) << "Redacted" << oldEvent->id() << "with" << redaction->id();
    emit q->replacedEvent(ti.event(), rawPtr(oldEvent));
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

void Room::Private::addNewMessageEvents(RoomEvents&& events)
{
    auto timelineSize = timeline.size();

    dropDuplicateEvents(&events);
    // We want to process redactions in the order of arrival (covering the
    // case of one redaction superseding another one), hence stable partition.
    const auto normalsBegin =
            stable_partition(events.begin(), events.end(), isRedaction);
    RoomEventsRange redactions { events.begin(), normalsBegin },
                    normalEvents { normalsBegin, events.end() };

    if (!normalEvents.empty())
        emit q->aboutToAddNewMessages(normalEvents);
    const auto insertedSize = insertEvents(move(normalEvents), Newer);
    const auto from = timeline.cend() - insertedSize;
    if (insertedSize > 0)
    {
        qCDebug(MAIN)
                << "Room" << displayname << "received" << insertedSize
                << "new events; the last event is now" << timeline.back();
        q->onAddNewTimelineEvents(from);
    }
    for (auto&& r: redactions)
    {
        Q_ASSERT(isRedaction(r));
        processRedaction(ptrCast<RedactionEvent>(move(r)));
    }
    if (insertedSize > 0)
    {
        emit q->addedMessages();

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

    Q_ASSERT(timeline.size() == timelineSize + insertedSize);
}

void Room::Private::addHistoricalMessageEvents(RoomEvents&& events)
{
    const auto timelineSize = timeline.size();

    dropDuplicateEvents(&events);
    const auto redactionsBegin =
            remove_if(events.begin(), events.end(), isRedaction);
    RoomEventsRange normalEvents { events.begin(), redactionsBegin };
    if (normalEvents.empty())
        return;

    emit q->aboutToAddHistoricalMessages(normalEvents);
    const auto insertedSize = insertEvents(move(normalEvents), Older);
    const auto from = timeline.crend() - insertedSize;

    qCDebug(MAIN) << "Room" << displayname << "received" << insertedSize
                  << "past events; the oldest event is now" << timeline.front();
    q->onAddHistoricalTimelineEvents(from);
    emit q->addedMessages();

    if (from <= q->readMarker())
        updateUnreadCount(from, timeline.crend());

    Q_ASSERT(timeline.size() == timelineSize + insertedSize);
}

bool Room::processStateEvent(const RoomEvent& e)
{
    switch (e.type())
    {
        case EventType::RoomName: {
            d->name = static_cast<const RoomNameEvent&>(e).name();
            qCDebug(MAIN) << "Room name updated:" << d->name;
            return true;
        }
        case EventType::RoomAliases: {
            d->aliases = static_cast<const RoomAliasesEvent&>(e).aliases();
            qCDebug(MAIN) << "Room aliases updated:" << d->aliases;
            return true;
        }
        case EventType::RoomCanonicalAlias: {
            d->canonicalAlias =
                    static_cast<const RoomCanonicalAliasEvent&>(e).alias();
            setObjectName(d->canonicalAlias);
            qCDebug(MAIN) << "Room canonical alias updated:" << d->canonicalAlias;
            return true;
        }
        case EventType::RoomTopic: {
            d->topic = static_cast<const RoomTopicEvent&>(e).topic();
            qCDebug(MAIN) << "Room topic updated:" << d->topic;
            emit topicChanged();
            return false;
        }
        case EventType::RoomAvatar: {
            const auto& avatarEventContent =
                    static_cast<const RoomAvatarEvent&>(e).content();
            if (d->avatar.updateUrl(avatarEventContent.url))
            {
                qCDebug(MAIN) << "Room avatar URL updated:"
                              << avatarEventContent.url.toString();
                emit avatarChanged();
            }
            return false;
        }
        case EventType::RoomMember: {
            const auto& memberEvent = static_cast<const RoomMemberEvent&>(e);
            auto* u = user(memberEvent.userId());
            u->processEvent(memberEvent, this);
            if (u == localUser() && memberJoinState(u) == JoinState::Invite
                    && memberEvent.isDirect())
                connection()->addToDirectChats(this,
                                user(memberEvent.senderId()));

            if( memberEvent.membership() == MembershipType::Join )
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
            else if( memberEvent.membership() == MembershipType::Leave )
            {
                if (memberJoinState(u) == JoinState::Join)
                {
                    if (!d->membersLeft.contains(u))
                        d->membersLeft.append(u);
                    d->removeMemberFromMap(u->name(this), u);
                    emit userRemoved(u);
                }
            }
            return false;
        }
        case EventType::RoomEncryption:
        {
            d->encryptionAlgorithm =
                    static_cast<const EncryptionEvent&>(e).algorithm();
            qCDebug(MAIN) << "Encryption switched on in" << displayName();
            emit encryption();
            return false;
        }
        default:
            /* Ignore events of other types */
            return false;
    }
}

void Room::processEphemeralEvent(EventPtr&& event)
{
    QElapsedTimer et; et.start();
    switch (event->type())
    {
        case EventType::Typing: {
            auto typingEvent = ptrCast<TypingEvent>(move(event));
            d->usersTyping.clear();
            for( const QString& userId: typingEvent->users() )
            {
                auto u = user(userId);
                if (memberJoinState(u) == JoinState::Join)
                    d->usersTyping.append(u);
            }
            if (!typingEvent->users().isEmpty())
                qCDebug(PROFILER) << "*** Room::processEphemeralEvent(typing):"
                    << typingEvent->users().size() << "users," << et;
            emit typingChanged();
            break;
        }
        case EventType::Receipt: {
            auto receiptEvent = ptrCast<ReceiptEvent>(move(event));
            for( const auto &p: receiptEvent->eventsWithReceipts() )
            {
                {
                    if (p.receipts.size() == 1)
                        qCDebug(EPHEMERAL) << "Marking" << p.evtId
                                           << "as read for" << p.receipts[0].userId;
                    else
                        qCDebug(EPHEMERAL) << "Marking" << p.evtId
                                           << "as read for"
                                           << p.receipts.size() << "users";
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
            if (!receiptEvent->eventsWithReceipts().isEmpty())
                qCDebug(PROFILER) << "*** Room::processEphemeralEvent(receipts):"
                    << receiptEvent->eventsWithReceipts().size()
                    << "events with receipts," << et;
            break;
        }
        default:
            qCWarning(EPHEMERAL) << "Unexpected event type in 'ephemeral' batch:"
                                 << event->jsonType();
    }
}

void Room::processAccountDataEvent(EventPtr&& event)
{
    switch (event->type())
    {
        case EventType::Tag:
        {
            auto newTags = ptrCast<const TagEvent>(move(event))->tags();
            if (newTags == d->tags)
                break;
            d->tags = newTags;
            qCDebug(MAIN) << "Room" << id() << "is tagged with:"
                          << tagNames().join(", ");
            emit tagsChanged();
            break;
        }
        case EventType::ReadMarker:
        {
            auto readEventId =
                    ptrCast<const ReadMarkerEvent>(move(event))->event_id();
            qCDebug(MAIN) << "Server-side read marker at" << readEventId;
            d->serverReadMarker = readEventId;
            const auto newMarker = findInTimeline(readEventId);
            if (newMarker != timelineEdge())
                d->markMessagesAsRead(newMarker);
            else
                d->setLastReadEvent(localUser(), readEventId);
            break;
        }
        default:
            d->accountData[event->jsonType()] =
                    fromJson<AccountDataMap>(event->contentJson());
            emit accountDataChanged(event->jsonType());
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
    // (the local user is apparently invited).
    if (userlist.size() == 1 && !isLocalUser(first_two.front()))
        return tr("Invitation from %1")
                .arg(q->roomMembername(first_two.front()));

    // i. One-on-one chat. first_two[1] == localUser() in this case.
    if (userlist.size() == 2)
        return q->roomMembername(first_two[0]);

    // ii. Two users besides the current one.
    if (userlist.size() == 3)
        return tr("%1 and %2")
                .arg(q->roomMembername(first_two[0]))
                .arg(q->roomMembername(first_two[1]));

    // iii. More users.
    if (userlist.size() > 3)
        return tr("%1 and %L2 others")
                .arg(q->roomMembername(first_two[0]))
                .arg(userlist.size() - 3);

    // userlist.size() < 2 - apparently, there's only current user in the room
    return QString();
}

QString Room::Private::calculateDisplayname() const
{
    // CS spec, section 11.2.2.5 Calculating the display name for a room
    // Numbers below refer to respective parts in the spec.

    // 1. Name (from m.room.name)
    if (!name.isEmpty()) {
        return name;
    }

    // 2. Canonical alias
    if (!canonicalAlias.isEmpty())
        return canonicalAlias;

    // 3. Room members
    QString topMemberNames = roomNameFromMemberNames(membersMap.values());
    if (!topMemberNames.isEmpty())
        return topMemberNames;

    // 4. Users that previously left the room
    topMemberNames = roomNameFromMemberNames(membersLeft);
    if (!topMemberNames.isEmpty())
        return tr("Empty room (was: %1)").arg(topMemberNames);

    // 5. Fail miserably
    return tr("Empty room (%1)").arg(id);

    // Using m.room.aliases is explicitly discouraged by the spec
    //if (!aliases.empty() && !aliases.at(0).isEmpty())
    //    displayname = aliases.at(0);
}

void Room::Private::updateDisplayname()
{
    const QString old_name = displayname;
    displayname = calculateDisplayname();
    if (old_name != displayname)
        emit q->displaynameChanged(q);
}

void appendStateEvent(QJsonArray& events, const QString& type,
                      const QJsonObject& content, const QString& stateKey = {})
{
    if (!content.isEmpty() || !stateKey.isEmpty())
        events.append(QJsonObject
            { { QStringLiteral("type"), type }
            , { QStringLiteral("content"), content }
            , { QStringLiteral("state_key"), stateKey }
            });
}

#define ADD_STATE_EVENT(events, type, name, content) \
    appendStateEvent((events), QStringLiteral(type), \
                     {{ QStringLiteral(name), content }});

void appendEvent(QJsonArray& events, const QString& type,
                 const QJsonObject& content)
{
    if (!content.isEmpty())
        events.append(QJsonObject
            { { QStringLiteral("type"), type }
            , { QStringLiteral("content"), content }
            });
}

template <typename EvtT>
void appendEvent(QJsonArray& events, const EvtT& event)
{
    appendEvent(events, EvtT::typeId(), event.toJson());
}

QJsonObject Room::Private::toJson() const
{
    QElapsedTimer et; et.start();
    QJsonObject result;
    {
        QJsonArray stateEvents;

        ADD_STATE_EVENT(stateEvents, "m.room.name", "name", name);
        ADD_STATE_EVENT(stateEvents, "m.room.topic", "topic", topic);
        ADD_STATE_EVENT(stateEvents, "m.room.avatar", "url",
                        avatar.url().toString());
        ADD_STATE_EVENT(stateEvents, "m.room.aliases", "aliases",
                        QJsonArray::fromStringList(aliases));
        ADD_STATE_EVENT(stateEvents, "m.room.canonical_alias", "alias",
                        canonicalAlias);
        ADD_STATE_EVENT(stateEvents, "m.room.encryption", "algorithm",
                        encryptionAlgorithm);

        for (const auto *m : membersMap)
            appendStateEvent(stateEvents, QStringLiteral("m.room.member"),
                { { QStringLiteral("membership"), QStringLiteral("join") }
                , { QStringLiteral("displayname"), m->rawName(q) }
                , { QStringLiteral("avatar_url"), m->avatarUrl(q).toString() }
            }, m->id());

        const auto stateObjName = joinState == JoinState::Invite ?
                    QStringLiteral("invite_state") : QStringLiteral("state");
        result.insert(stateObjName,
            QJsonObject {{ QStringLiteral("events"), stateEvents }});
    }

    QJsonArray accountDataEvents;
    if (!tags.empty())
        appendEvent(accountDataEvents, TagEvent(tags));

    if (!serverReadMarker.isEmpty())
        appendEvent(accountDataEvents, ReadMarkerEvent(serverReadMarker));

    if (!accountData.empty())
    {
        for (auto it = accountData.begin(); it != accountData.end(); ++it)
            appendEvent(accountDataEvents, it.key(),
                        QMatrixClient::toJson(it.value()));
    }
    result.insert("account_data", QJsonObject {{ "events", accountDataEvents }});

    QJsonObject unreadNotificationsObj;

    unreadNotificationsObj.insert(SyncRoomData::UnreadCountKey, unreadMessages);
    if (highlightCount > 0)
        unreadNotificationsObj.insert("highlight_count", highlightCount);
    if (notificationCount > 0)
        unreadNotificationsObj.insert("notification_count", notificationCount);

    result.insert("unread_notifications", unreadNotificationsObj);

    if (et.elapsed() > 50)
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

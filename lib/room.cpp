// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-FileCopyrightText: 2018 Black Hat <bhat@encom.eu.org>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-FileCopyrightText: 2020 Ram Nad <ramnad1999@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "room.h"

#include "avatar.h"
#include "connection.h"
#include "converters.h"
#include "crypto/e2ee.h"
#include "syncdata.h"
#include "user.h"
#include "eventstats.h"

// NB: since Qt 6, moc_room.cpp needs User fully defined
#include "moc_room.cpp"

#include "csapi/account-data.h"
#include "csapi/banning.h"
#include "csapi/inviting.h"
#include "csapi/kicking.h"
#include "csapi/leaving.h"
#include "csapi/receipts.h"
#include "csapi/redaction.h"
#include "csapi/room_send.h"
#include "csapi/room_state.h"
#include "csapi/room_upgrades.h"
#include "csapi/rooms.h"
#include "csapi/read_markers.h"
#include "csapi/tags.h"

#include "events/callanswerevent.h"
#include "events/callcandidatesevent.h"
#include "events/callhangupevent.h"
#include "events/callinviteevent.h"
#include "events/encryptionevent.h"
#include "events/reactionevent.h"
#include "events/receiptevent.h"
#include "events/redactionevent.h"
#include "events/roomavatarevent.h"
#include "events/roomcreateevent.h"
#include "events/roommemberevent.h"
#include "events/roomtombstoneevent.h"
#include "events/simplestateevents.h"
#include "events/typingevent.h"
#include "events/roompowerlevelsevent.h"
#include "jobs/downloadfilejob.h"
#include "jobs/mediathumbnailjob.h"
#include "events/roomcanonicalaliasevent.h"

#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder> // for efficient string concats (operator%)
#include <QtCore/QTemporaryFile>

#include <array>
#include <cmath>
#include <functional>

#ifdef Quotient_E2EE_ENABLED
#include "crypto/qolmaccount.h"
#include "crypto/qolmerrors.h"
#include "crypto/qolminboundsession.h"
#endif // Quotient_E2EE_ENABLED

using namespace Quotient;
using namespace std::placeholders;
using std::move;
#if !(defined __GLIBCXX__ && __GLIBCXX__ <= 20150123)
using std::llround;
#endif

enum EventsPlacement : int { Older = -1, Newer = 1 };

class Room::Private {
public:
    /// Map of user names to users
    /** User names potentially duplicate, hence QMultiHash. */
    using members_map_t = QMultiHash<QString, User*>;

    Private(Connection* c, QString id_, JoinState initialJoinState)
        : q(nullptr), connection(c), id(move(id_)), joinState(initialJoinState)
    {}

    Room* q;

    Connection* connection;
    QString id;
    JoinState joinState;
    RoomSummary summary = { none, 0, none };
    /// The state of the room at timeline position before-0
    /// \sa timelineBase
    UnorderedMap<StateEventKey, StateEventPtr> baseState;
    /// State event stubs - events without content, just type and state key
    static decltype(baseState) stubbedState;
    /// The state of the room at syncEdge()
    /// \sa syncEdge
    QHash<StateEventKey, const StateEventBase*> currentState;
    /// Servers with aliases for this room except the one of the local user
    /// \sa Room::remoteAliases
    QSet<QString> aliasServers;

    Timeline timeline;
    PendingEvents unsyncedEvents;
    QHash<QString, TimelineItem::index_t> eventsIndex;
    // A map from evtId to a map of relation type to a vector of event
    // pointers. Not using QMultiHash, because we want to quickly return
    // a number of relations for a given event without enumerating them.
    QHash<QPair<QString, QString>, RelatedEvents> relations;
    QString displayname;
    Avatar avatar;
    QHash<QString, Notification> notifications;
    qsizetype serverHighlightCount = 0;
    // Starting up with estimate event statistics as there's zero knowledge
    // about the timeline.
    EventStats partiallyReadStats {}, unreadStats {};
    members_map_t membersMap;
    QList<User*> usersTyping;
    QHash<QString, QSet<QString>> eventIdReadUsers;
    QList<User*> usersInvited;
    QList<User*> membersLeft;
    bool displayed = false;
    QString firstDisplayedEventId;
    QString lastDisplayedEventId;
    QHash<QString, ReadReceipt> lastReadReceipts;
    QString fullyReadUntilEventId;
    TagsMap tags;
    UnorderedMap<QString, EventPtr> accountData;
    QString prevBatch;
    QPointer<GetRoomEventsJob> eventsHistoryJob;
    QPointer<GetMembersByRoomJob> allMembersJob;

    struct FileTransferPrivateInfo {
        FileTransferPrivateInfo() = default;
        FileTransferPrivateInfo(BaseJob* j, const QString& fileName,
                                bool isUploading = false)
            : status(FileTransferInfo::Started)
            , job(j)
            , localFileInfo(fileName)
            , isUpload(isUploading)
        {}

        FileTransferInfo::Status status = FileTransferInfo::None;
        QPointer<BaseJob> job = nullptr;
        QFileInfo localFileInfo {};
        bool isUpload = false;
        qint64 progress = 0;
        qint64 total = -1;

        void update(qint64 p, qint64 t)
        {
            if (t == 0) {
                t = -1;
                if (p == 0)
                    p = -1;
            }
            if (p != -1)
                qCDebug(PROFILER) << "Transfer progress:" << p << "/" << t
                                  << "=" << llround(double(p) / t * 100) << "%";
            progress = p;
            total = t;
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

    Changes setSummary(RoomSummary&& newSummary);

    // void inviteUser(User* u); // We might get it at some point in time.
    void insertMemberIntoMap(User* u);
    void removeMemberFromMap(User* u);

    // This updates the room displayname field (which is the way a room
    // should be shown in the room list); called whenever the list of
    // members, the room name (m.room.name) or canonical alias change.
    void updateDisplayname();
    // This is used by updateDisplayname() but only calculates the new name
    // without any updates.
    QString calculateDisplayname() const;

    /// A point in the timeline corresponding to baseState
    rev_iter_t timelineBase() const { return q->findInTimeline(-1); }
    rev_iter_t historyEdge() const { return timeline.crend(); }
    Timeline::const_iterator syncEdge() const { return timeline.cend(); }

    void getPreviousContent(int limit = 10, const QString &filter = {});

    const StateEventBase* getCurrentState(const StateEventKey& evtKey) const
    {
        const auto* evt = currentState.value(evtKey, nullptr);
        if (!evt) {
            if (stubbedState.find(evtKey) == stubbedState.end()) {
                // In the absence of a real event, make a stub as-if an event
                // with empty content has been received. Event classes should be
                // prepared for empty/invalid/malicious content anyway.
                stubbedState.emplace(evtKey, loadStateEvent(evtKey.first, {},
                                                            evtKey.second));
                qCDebug(STATE) << "A new stub event created for key {"
                               << evtKey.first << evtKey.second << "}";
            }
            evt = stubbedState[evtKey].get();
            Q_ASSERT(evt);
        }
        Q_ASSERT(evt->matrixType() == evtKey.first
                 && evt->stateKey() == evtKey.second);
        return evt;
    }

    QVector<const StateEventBase*> stateEventsOfType(const QString& evtType) const
    {
        auto vals = QVector<const StateEventBase*>();
        for (auto it = currentState.cbegin(); it != currentState.cend(); ++it)
            if (it.key().first == evtType)
                vals.append(it.value());

        return vals;
    }

    template <typename EventT>
    const EventT* getCurrentState(const QString& stateKey = {}) const
    {
        const auto* evt = getCurrentState({ EventT::matrixTypeId(), stateKey });
        Q_ASSERT(evt->type() == EventT::typeId()
                 && evt->matrixType() == EventT::matrixTypeId());
        return static_cast<const EventT*>(evt);
    }

//    template <typename EventT>
//    const auto& getCurrentStateContent(const QString& stateKey = {}) const
//    {
//        if (const auto* evt =
//                currentState.value({ EventT::matrixTypeId(), stateKey }, nullptr))
//            return evt->content();
//        return EventT::content_type()
//    }

    template <typename EventArrayT>
    Changes updateStateFrom(EventArrayT&& events)
    {
        Changes changes {};
        if (!events.empty()) {
            QElapsedTimer et;
            et.start();
            for (auto&& eptr : events) {
                const auto& evt = *eptr;
                Q_ASSERT(evt.isStateEvent());
                if (auto change = q->processStateEvent(evt); change) {
                    changes |= change;
                    baseState[{ evt.matrixType(), evt.stateKey() }] = move(eptr);
                }
            }
            if (events.size() > 9 || et.nsecsElapsed() >= profilerMinNsecs())
                qCDebug(PROFILER)
                    << "Updated" << q->objectName() << "room state from"
                    << events.size() << "event(s) in" << et;
        }
        return changes;
    }
    Changes addNewMessageEvents(RoomEvents&& events);
    void addHistoricalMessageEvents(RoomEvents&& events);

    Changes updateStatsFromSyncData(const SyncRoomData &data, bool fromCache);
    void postprocessChanges(Changes changes, bool saveState = true);

    /** Move events into the timeline
     *
     * Insert events into the timeline, either new or historical.
     * Pointers in the original container become empty, the ownership
     * is passed to the timeline container.
     * @param events - the range of events to be inserted
     * @param placement - position and direction of insertion: Older for
     *                    historical messages, Newer for new ones
     */
    Timeline::size_type moveEventsToTimeline(RoomEventsRange events,
                                             EventsPlacement placement);

    /**
     * Remove events from the passed container that are already in the timeline
     */
    void dropDuplicateEvents(RoomEvents& events) const;

    Changes setLastReadReceipt(const QString& userId, rev_iter_t newMarker,
                               ReadReceipt newReceipt = {},
                               bool deferStatsUpdate = false);
    Changes setFullyReadMarker(const QString &eventId);
    Changes updateStats(const rev_iter_t& from, const rev_iter_t& to);
    bool markMessagesAsRead(const rev_iter_t& upToMarker);

    void getAllMembers();

    QString sendEvent(RoomEventPtr&& event);

    template <typename EventT, typename... ArgTs>
    QString sendEvent(ArgTs&&... eventArgs)
    {
        return sendEvent(makeEvent<EventT>(std::forward<ArgTs>(eventArgs)...));
    }

    QString doPostFile(RoomEventPtr &&msgEvent, const QUrl &localUrl);

    RoomEvent* addAsPending(RoomEventPtr&& event);

    QString doSendEvent(const RoomEvent* pEvent);
    void onEventSendingFailure(const QString& txnId, BaseJob* call = nullptr);

    SetRoomStateWithKeyJob* requestSetState(const StateEventBase& event)
    {
        //            if (event.roomId().isEmpty())
        //                event.setRoomId(id);
        //            if (event.senderId().isEmpty())
        //                event.setSender(connection->userId());
        // TODO: Queue up state events sending (see #133).
        // TODO: Maybe addAsPending() as well, despite having no txnId
        return connection->callApi<SetRoomStateWithKeyJob>(
            id, event.matrixType(), event.stateKey(), event.contentJson());
    }

    template <typename EvT, typename... ArgTs>
    auto requestSetState(ArgTs&&... args)
    {
        return requestSetState(EvT(std::forward<ArgTs>(args)...));
    }

    /*! Apply redaction to the timeline
     *
     * Tries to find an event in the timeline and redact it; deletes the
     * redaction event whether the redacted event was found or not.
     * \return true if the event has been found and redacted; false otherwise
     */
    bool processRedaction(const RedactionEvent& redaction);

    /*! Apply a new revision of the event to the timeline
     *
     * Tries to find an event in the timeline and replace it with the new
     * content passed in \p newMessage.
     * \return true if the event has been found and replaced; false otherwise
     */
    bool processReplacement(const RoomMessageEvent& newEvent);

    void setTags(TagsMap&& newTags);

    QJsonObject toJson() const;

    bool isLocalUser(const User* u) const { return u == q->localUser(); }

#ifdef Quotient_E2EE_ENABLED
    // A map from <sessionId, messageIndex> to <event_id, origin_server_ts>
    QHash<QPair<QString, uint32_t>, QPair<QString, QDateTime>>
        groupSessionIndexRecord; // TODO: cache
    // A map from senderKey to a map of sessionId to InboundGroupSession
    // Not using QMultiHash, because we want to quickly return
    // a number of relations for a given event without enumerating them.
    QHash<QPair<QString, QString>, QOlmInboundGroupSession*> groupSessions; // TODO:
                                                                        // cache
    bool addInboundGroupSession(QString senderKey, QString sessionId,
                                QString sessionKey)
    {
        // new e2ee TODO:
        /*
        if (groupSessions.contains({ senderKey, sessionId })) {
            qCDebug(E2EE) << "Inbound Megolm session" << sessionId
                          << "with senderKey" << senderKey << "already exists";
            return false;
        }

        QOlmInboundGroupSession* megolmSession;
        try {
            megolmSession = new QOlmInboundGroupSession(sessionKey.toLatin1(),
                                                    InboundGroupSession::Init,
                                                    q);
        } catch (QOlmError* e) {
            qCDebug(E2EE) << "Unable to create new InboundGroupSession"
                          << e->what();
            return false;
        }
        if (megolmSession->id() != sessionId) {
            qCDebug(E2EE) << "Session ID mismatch in m.room_key event sent "
                             "from sender with key"
                          << senderKey;
            return false;
        }
        groupSessions.insert({ senderKey, sessionId }, megolmSession);
        */
        return true;
    }

    QString groupSessionDecryptMessage(QByteArray cipher,
                                       const QString& senderKey,
                                       const QString& sessionId,
                                       const QString& eventId,
                                       QDateTime timestamp)
    {
        std::pair<QString, uint32_t> decrypted;
        // new e2ee TODO:
        /*
        QPair<QString, QString> senderSessionPairKey =
            qMakePair(senderKey, sessionId);
        if (!groupSessions.contains(senderSessionPairKey)) {
            qCDebug(E2EE) << "Unable to decrypt event" << eventId
                          << "The sender's device has not sent us the keys for "
                             "this message";
            return QString();
        }
        QOlmInboundGroupSession* senderSession =
            groupSessions.value(senderSessionPairKey);
        if (!senderSession) {
            qCDebug(E2EE) << "Unable to decrypt event" << eventId
                          << "senderSessionPairKey:" << senderSessionPairKey;
            return QString();
        }
        try {
            decrypted = senderSession->decrypt(cipher);
        } catch (QOlmError* e) {
            qCDebug(E2EE) << "Unable to decrypt event" << eventId
                          << "with matching megolm session:" << e->what();
            return QString();
        }
        QPair<QString, QDateTime> properties = groupSessionIndexRecord.value(
            qMakePair(senderSession->id(), decrypted.second));
        if (properties.first.isEmpty()) {
            groupSessionIndexRecord.insert(qMakePair(senderSession->id(),
                                                     decrypted.second),
                                           qMakePair(eventId, timestamp));
        } else {
            if ((properties.first != eventId)
                || (properties.second != timestamp)) {
                qCDebug(E2EE) << "Detected a replay attack on event" << eventId;
                return QString();
            }
        }
        */

        return decrypted.first;
    }
#endif // Quotient_E2EE_ENABLED

private:
    using users_shortlist_t = std::array<User*, 3>;
    template <typename ContT>
    users_shortlist_t buildShortlist(const ContT& users) const;
    users_shortlist_t buildShortlist(const QStringList& userIds) const;
};

decltype(Room::Private::baseState) Room::Private::stubbedState {};

Room::Room(Connection* connection, QString id, JoinState initialJoinState)
    : QObject(connection), d(new Private(connection, id, initialJoinState))
{
    setObjectName(id);
    // See "Accessing the Public Class" section in
    // https://marcmutz.wordpress.com/translated-articles/pimp-my-pimpl-%E2%80%94-reloaded/
    d->q = this;
    d->displayname = d->calculateDisplayname(); // Set initial "Empty room" name
    connectUntil(connection, &Connection::loadedRoomState, this, [this](Room* r) {
        if (this == r)
            emit baseStateLoaded();
        return this == r; // loadedRoomState fires only once per room
    });
    connectSingleShot(this, &Room::encryption, this, [=](){
        connection->newEncryptedRoom(this);
    });
    qCDebug(STATE) << "New" << terse << initialJoinState << "Room:" << id;
}

Room::~Room() { delete d; }

const QString& Room::id() const { return d->id; }

QString Room::version() const
{
    const auto v = d->getCurrentState<RoomCreateEvent>()->version();
    return v.isEmpty() ? QStringLiteral("1") : v;
}

bool Room::isUnstable() const
{
    return !connection()->loadingCapabilities()
           && !connection()->stableRoomVersions().contains(version());
}

QString Room::predecessorId() const
{
    return d->getCurrentState<RoomCreateEvent>()->predecessor().roomId;
}

Room* Room::predecessor(JoinStates statesFilter) const
{
    if (const auto& predId = predecessorId(); !predId.isEmpty())
        if (auto* r = connection()->room(predId, statesFilter);
                r && r->successorId() == id())
            return r;

    return nullptr;
}

QString Room::successorId() const
{
    return d->getCurrentState<RoomTombstoneEvent>()->successorRoomId();
}

Room* Room::successor(JoinStates statesFilter) const
{
    if (const auto& succId = successorId(); !succId.isEmpty())
        if (auto* r = connection()->room(succId, statesFilter);
                r && r->predecessorId() == id())
            return r;

    return nullptr;
}

const Room::Timeline& Room::messageEvents() const { return d->timeline; }

const Room::PendingEvents& Room::pendingEvents() const
{
    return d->unsyncedEvents;
}

bool Room::allHistoryLoaded() const
{
    return !d->timeline.empty() && is<RoomCreateEvent>(*d->timeline.front());
}

QString Room::name() const
{
    return d->getCurrentState<RoomNameEvent>()->name();
}

QStringList Room::aliases() const
{
    const auto* evt = d->getCurrentState<RoomCanonicalAliasEvent>();
    auto result = evt->altAliases();
    if (!evt->alias().isEmpty())
        result << evt->alias();
    return result;
}

QStringList Room::altAliases() const
{
    return d->getCurrentState<RoomCanonicalAliasEvent>()->altAliases();
}

QString Room::canonicalAlias() const
{
    return d->getCurrentState<RoomCanonicalAliasEvent>()->alias();
}

QString Room::displayName() const { return d->displayname; }

QString Room::displayNameForHtml() const
{
    return displayName().toHtmlEscaped();
}

void Room::refreshDisplayName() { d->updateDisplayname(); }

QString Room::topic() const
{
    return d->getCurrentState<RoomTopicEvent>()->topic();
}

QString Room::avatarMediaId() const { return d->avatar.mediaId(); }

QUrl Room::avatarUrl() const { return d->avatar.url(); }

const Avatar& Room::avatarObject() const { return d->avatar; }

QImage Room::avatar(int dimension) { return avatar(dimension, dimension); }

QImage Room::avatar(int width, int height)
{
    if (!d->avatar.url().isEmpty())
        return d->avatar.get(connection(), width, height,
                             [this] { emit avatarChanged(); });

    // Use the first (excluding self) user's avatar for direct chats
    const auto dcUsers = directChatUsers();
    for (auto* u : dcUsers)
        if (u != localUser())
            return u->avatar(width, height, this, [this] { emit avatarChanged(); });

    return {};
}

User* Room::user(const QString& userId) const
{
    return connection()->user(userId);
}

JoinState Room::memberJoinState(User* user) const
{
    return d->membersMap.contains(user->name(this), user) ? JoinState::Join
                                                          : JoinState::Leave;
}

Membership Room::memberState(const QString& userId) const
{
    return d->getCurrentState<RoomMemberEvent>(userId)->membership();
}

bool Room::isMember(const QString& userId) const
{
    return memberState(userId) == Membership::Join;
}

JoinState Room::joinState() const { return d->joinState; }

void Room::setJoinState(JoinState state)
{
    JoinState oldState = d->joinState;
    if (state == oldState)
        return;
    d->joinState = state;
    qCDebug(STATE) << "Room" << id() << "changed state: " << terse << oldState
                   << "->" << state;
    emit joinStateChanged(oldState, state);
}

Room::Changes Room::Private::setLastReadReceipt(const QString& userId,
                                                rev_iter_t newMarker,
                                                ReadReceipt newReceipt,
                                                bool deferStatsUpdate)
{
    if (newMarker == historyEdge() && !newReceipt.eventId.isEmpty())
        newMarker = q->findInTimeline(newReceipt.eventId);
    if (newMarker != historyEdge()) {
        // Try to auto-promote the read marker over the user's own messages
        // (switch to direct iterators for that).
        const auto eagerMarker = find_if(newMarker.base(), syncEdge(),
                                         [=](const TimelineItem& ti) {
                                             return ti->senderId() != userId;
                                         });
        // eagerMarker is now just after the desired event for newMarker
        if (eagerMarker != newMarker.base()) {
            newMarker = rev_iter_t(eagerMarker);
            qCDebug(EPHEMERAL) << "Auto-promoted read receipt for" << userId
                               << "to" << *newMarker;
        }
        // Fill newReceipt with the event (and, if needed, timestamp) from
        // eagerMarker
        newReceipt.eventId = (eagerMarker - 1)->event()->id();
        if (newReceipt.timestamp.isNull())
            newReceipt.timestamp = QDateTime::currentDateTime();
    }
    auto& storedReceipt =
            lastReadReceipts[userId]; // clazy:exclude=detaching-member
    const auto prevEventId = storedReceipt.eventId;
    // Check that either the new marker is actually "newer" than the current one
    // or, if both markers are at historyEdge(), event ids are different.
    // NB: with reverse iterators, timeline history edge >= sync edge
    if (prevEventId == newReceipt.eventId
        || newMarker > q->findInTimeline(prevEventId))
        return Change::None;

    // Finally make the change

    Changes changes = Change::Other;
    auto oldEventReadUsersIt =
        eventIdReadUsers.find(prevEventId); // clazy:exclude=detaching-member
    if (oldEventReadUsersIt != eventIdReadUsers.end()) {
        oldEventReadUsersIt->remove(userId);
        if (oldEventReadUsersIt->isEmpty())
            eventIdReadUsers.erase(oldEventReadUsersIt);
    }
    eventIdReadUsers[newReceipt.eventId].insert(userId);
    storedReceipt = move(newReceipt);

    {
        auto dbg = qDebug(EPHEMERAL); // This trick needs qDebug, not qCDebug
        dbg << "The new read receipt for" << userId << "is now at";
        if (newMarker == historyEdge())
            dbg << storedReceipt.eventId;
        else
            dbg << *newMarker;
    }

    // TODO: use Room::member() when it becomes a thing and only emit signals
    //       for actual members, not just any user
    const auto member = q->user(userId);
    Q_ASSERT(member != nullptr);
    if (isLocalUser(member) && !deferStatsUpdate) {
        if (unreadStats.updateOnMarkerMove(q, q->findInTimeline(prevEventId),
                                           newMarker)) {
            qCDebug(MESSAGES)
                << "Updated unread event statistics in" << q->objectName()
                << "after moving the local read receipt:" << unreadStats;
            changes |= Change::UnreadStats;
        }
        Q_ASSERT(unreadStats.isValidFor(q, newMarker)); // post-check
    }
    emit q->lastReadEventChanged(member);
    // TODO: remove in 0.8
    if (!isLocalUser(member))
        emit q->readMarkerForUserMoved(member, prevEventId,
                                       storedReceipt.eventId);
    return changes;
}

Room::Changes Room::Private::updateStats(const rev_iter_t& from,
                                         const rev_iter_t& to)
{
    Q_ASSERT(from >= timeline.crbegin() && from <= timeline.crend());
    Q_ASSERT(to >= from && to <= timeline.crend());

    const auto fullyReadMarker = q->fullyReadMarker();
    auto readReceiptMarker = q->localReadReceiptMarker();
    Changes changes = Change::None;
    // Correct the read receipt to never be behind the fully read marker
    if (readReceiptMarker > fullyReadMarker
        && setLastReadReceipt(connection->userId(), fullyReadMarker, {}, true)) {
        changes |= Change::Other;
        readReceiptMarker = q->localReadReceiptMarker();
        qCInfo(MESSAGES) << "The local m.read receipt was behind m.fully_read "
                            "marker - it's now corrected to be at index"
                         << readReceiptMarker->index();
    }

    if (fullyReadMarker < from)
        return Change::None; // What's arrived is already fully read

    // If there's no read marker in the whole room, initialise it
    if (fullyReadMarker == historyEdge() && q->allHistoryLoaded())
        return setFullyReadMarker(timeline.front()->id());

    // Catch a case when the id in the last fully read marker or the local read
    // receipt refers to an event that has just arrived. In this case either
    // one (unreadStats) or both statistics should be recalculated to get
    // an exact number instead of an estimation (see documentation on
    // EventStats::isEstimate). For the same reason (switching from the
    // estimate to the exact number) this branch forces returning
    // Change::UnreadStats and also possibly Change::PartiallyReadStats, even if
    // the estimation luckily matched the exact result.
    if (readReceiptMarker < to || changes /*i.e. read receipt was corrected*/) {
        unreadStats = EventStats::fromMarker(q, readReceiptMarker);
        Q_ASSERT(!unreadStats.isEstimate);
        qCDebug(MESSAGES).nospace() << "Recalculated unread event statistics in"
                                    << q->objectName() << ": " << unreadStats;
        changes |= Change::UnreadStats;
        if (fullyReadMarker < to) {
            // Add up to unreadStats instead of counting same events again
            partiallyReadStats = EventStats::fromRange(q, readReceiptMarker,
                                                       q->fullyReadMarker(),
                                                       unreadStats);
            Q_ASSERT(!partiallyReadStats.isEstimate);

            qCDebug(MESSAGES).nospace()
                    << "Recalculated partially read event statistics in "
                    << q->objectName() << ": " << partiallyReadStats;
            return changes | Change::PartiallyReadStats;
        }
    }

    // As of here, at least the fully read marker (but maybe also read receipt)
    // points to somewhere beyond the "oldest" message from the arrived batch -
    // add up newly arrived messages to the current stats, instead of a complete
    // recalculation.
    Q_ASSERT(fullyReadMarker >= to);

    const auto newStats = EventStats::fromRange(q, from, to);
    Q_ASSERT(!newStats.isEstimate);
    if (newStats.empty())
        return changes;

    const auto doAddStats = [this, &changes, newStats](EventStats& s,
                                                       const rev_iter_t& marker,
                                                       Change c) {
        s.notableCount += newStats.notableCount;
        s.highlightCount += newStats.highlightCount;
        if (!s.isEstimate)
            s.isEstimate = marker == historyEdge();
        changes |= c;
    };

    doAddStats(partiallyReadStats, fullyReadMarker, Change::PartiallyReadStats);
    if (readReceiptMarker >= to) {
        // readReceiptMarker < to branch shouldn't have been entered
        Q_ASSERT(!changes.testFlag(Change::UnreadStats));
        doAddStats(unreadStats, readReceiptMarker, Change::UnreadStats);
    }
    qCDebug(MESSAGES) << "Room" << q->objectName() << "has gained" << newStats
                      << "notable/highlighted event(s); total statistics:"
                      << partiallyReadStats << "since the fully read marker,"
                      << unreadStats << "since read receipt";

    // Check invariants
    Q_ASSERT(partiallyReadStats.isValidFor(q, fullyReadMarker));
    Q_ASSERT(unreadStats.isValidFor(q, readReceiptMarker));
    return changes;
}

Room::Changes Room::Private::setFullyReadMarker(const QString& eventId)
{
    if (fullyReadUntilEventId == eventId)
        return Change::None;

    const auto prevReadMarker = q->fullyReadMarker();
    const auto newReadMarker = q->findInTimeline(eventId);
    if (newReadMarker > prevReadMarker)
        return Change::None;

    const auto prevFullyReadId = std::exchange(fullyReadUntilEventId, eventId);
    qCDebug(MESSAGES) << "Fully read marker in" << q->objectName() //
                      << "set to" << fullyReadUntilEventId;

    QT_IGNORE_DEPRECATIONS(Changes changes = Change::ReadMarker|Change::Other;)
    if (const auto rm = q->fullyReadMarker(); rm != historyEdge()) {
        // Pull read receipt if it's behind, and update statistics
        changes |= setLastReadReceipt(connection->userId(), rm);
        if (partiallyReadStats.updateOnMarkerMove(q, prevReadMarker, rm)) {
            changes |= Change::PartiallyReadStats;
            qCDebug(MESSAGES)
                << "Updated partially read event statistics in"
                << q->objectName()
                << "after moving m.fully_read marker: " << partiallyReadStats;
        }
        Q_ASSERT(partiallyReadStats.isValidFor(q, rm)); // post-check
    }
    emit q->fullyReadMarkerMoved(prevFullyReadId, fullyReadUntilEventId);
    // TODO: Remove in 0.8
    emit q->readMarkerMoved(prevFullyReadId, fullyReadUntilEventId);
    return changes;
}

void Room::setReadReceipt(const QString& atEventId)
{
    if (const auto changes = d->setLastReadReceipt(localUser()->id(),
                                                   historyEdge(),
                                                   { atEventId })) {
        connection()->callApi<PostReceiptJob>(BackgroundRequest, id(),
                                              QStringLiteral("m.read"),
                                              QUrl::toPercentEncoding(atEventId));
        d->postprocessChanges(changes);
    } else
        qCDebug(EPHEMERAL) << "The new read receipt for" << localUser()->id()
                           << "in" << objectName()
                           << "is at or behind the old one, skipping";
}

bool Room::Private::markMessagesAsRead(const rev_iter_t &upToMarker)
{
    if (upToMarker == q->historyEdge())
        qCWarning(MESSAGES) << "Cannot mark an unknown event in"
                            << q->objectName() << "as fully read";
    else if (const auto changes = setFullyReadMarker(upToMarker->event()->id())) {
        // The assumption below is that if a read receipt was sent on a newer
        // event, the homeserver will keep it there instead of reverting to
        // m.fully_read
        connection->callApi<SetReadMarkerJob>(BackgroundRequest, id,
                                              fullyReadUntilEventId,
                                              fullyReadUntilEventId);
        postprocessChanges(changes);
        return true;
    } else
        qCDebug(MESSAGES) << "Event" << *upToMarker << "in" << q->objectName()
                          << "is behind the current fully read marker at"
                          << *q->fullyReadMarker()
                          << "- won't move fully read marker back in timeline";
    return false;
}

void Room::markMessagesAsRead(const QString& uptoEventId)
{
    d->markMessagesAsRead(findInTimeline(uptoEventId));
}

void Room::markAllMessagesAsRead()
{
    d->markMessagesAsRead(d->timeline.crbegin());
}

bool Room::canSwitchVersions() const
{
    if (!successorId().isEmpty())
        return false; // No one can upgrade a room that's already upgraded

    if (const auto* plEvt = d->getCurrentState<RoomPowerLevelsEvent>()) {
        const auto currentUserLevel = plEvt->powerLevelForUser(localUser()->id());
        const auto tombstonePowerLevel =
            plEvt->powerLevelForState("m.room.tombstone"_ls);
        return currentUserLevel >= tombstonePowerLevel;
    }
    return true;
}

bool Room::isEventNotable(const TimelineItem &ti) const
{
    const auto& evt = *ti;
    const auto* rme = ti.viewAs<RoomMessageEvent>();
    return !evt.isRedacted()
           && (is<RoomTopicEvent>(evt) || is<RoomNameEvent>(evt)
               || is<RoomAvatarEvent>(evt) || is<RoomTombstoneEvent>(evt)
               || (rme && rme->msgtype() != MessageEventType::Notice
                   && rme->replacedEvent().isEmpty()))
           && evt.senderId() != localUser()->id();
}

Notification Room::notificationFor(const TimelineItem &ti) const
{
    return d->notifications.value(ti->id());
}

Notification Room::checkForNotifications(const TimelineItem &ti)
{
    return { Notification::None };
}

bool Room::hasUnreadMessages() const { return !d->partiallyReadStats.empty(); }

int countFromStats(const EventStats& s)
{
    return s.empty() ? -1 : int(s.notableCount);
}

int Room::unreadCount() const { return countFromStats(partiallyReadStats()); }

EventStats Room::partiallyReadStats() const { return d->partiallyReadStats; }

EventStats Room::unreadStats() const { return d->unreadStats; }

Room::rev_iter_t Room::historyEdge() const { return d->historyEdge(); }

Room::Timeline::const_iterator Room::syncEdge() const { return d->syncEdge(); }

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
    return !d->timeline.empty() && timelineIndex >= minTimelineIndex()
           && timelineIndex <= maxTimelineIndex();
}

Room::rev_iter_t Room::findInTimeline(TimelineItem::index_t index) const
{
    return historyEdge()
           - (isValidIndex(index) ? index - minTimelineIndex() + 1 : 0);
}

Room::rev_iter_t Room::findInTimeline(const QString& evtId) const
{
    if (!d->timeline.empty() && d->eventsIndex.contains(evtId)) {
        auto it = findInTimeline(d->eventsIndex.value(evtId));
        Q_ASSERT(it != historyEdge() && (*it)->id() == evtId);
        return it;
    }
    return historyEdge();
}

Room::PendingEvents::iterator Room::findPendingEvent(const QString& txnId)
{
    return std::find_if(d->unsyncedEvents.begin(), d->unsyncedEvents.end(),
                        [txnId](const auto& item) {
                            return item->transactionId() == txnId;
                        });
}

Room::PendingEvents::const_iterator
Room::findPendingEvent(const QString& txnId) const
{
    return std::find_if(d->unsyncedEvents.cbegin(), d->unsyncedEvents.cend(),
                        [txnId](const auto& item) {
                            return item->transactionId() == txnId;
                        });
}

const Room::RelatedEvents Room::relatedEvents(const QString& evtId,
                                              const char* relType) const
{
    return d->relations.value({ evtId, relType });
}

const Room::RelatedEvents Room::relatedEvents(const RoomEvent& evt,
                                              const char* relType) const
{
    return relatedEvents(evt.id(), relType);
}

void Room::Private::getAllMembers()
{
    // If already loaded or already loading, there's nothing to do here.
    if (q->joinedCount() <= membersMap.size() || isJobPending(allMembersJob))
        return;

    allMembersJob = connection->callApi<GetMembersByRoomJob>(
        id, connection->nextBatchToken(), "join");
    auto nextIndex = timeline.empty() ? 0 : timeline.back().index() + 1;
    connect(allMembersJob, &BaseJob::success, q, [this, nextIndex] {
        Q_ASSERT(timeline.empty() || nextIndex <= q->maxTimelineIndex() + 1);
        auto roomChanges = updateStateFrom(allMembersJob->chunk());
        // Replay member events that arrived after the point for which
        // the full members list was requested.
        if (!timeline.empty())
            for (auto it = q->findInTimeline(nextIndex).base();
                 it != syncEdge(); ++it)
                if (is<RoomMemberEvent>(**it))
                    roomChanges |= q->processStateEvent(**it);
        postprocessChanges(roomChanges);
        emit q->allMembersLoaded();
    });
}

bool Room::displayed() const { return d->displayed; }

void Room::setDisplayed(bool displayed)
{
    if (d->displayed == displayed)
        return;

    d->displayed = displayed;
    emit displayedChanged(displayed);
    if (displayed)
        d->getAllMembers();
}

QString Room::firstDisplayedEventId() const { return d->firstDisplayedEventId; }

Room::rev_iter_t Room::firstDisplayedMarker() const
{
    return findInTimeline(firstDisplayedEventId());
}

void Room::setFirstDisplayedEventId(const QString& eventId)
{
    if (d->firstDisplayedEventId == eventId)
        return;

    if (!eventId.isEmpty() && findInTimeline(eventId) == historyEdge())
        qCWarning(MESSAGES)
            << eventId
            << "is marked as first displayed but doesn't seem to be loaded";

    d->firstDisplayedEventId = eventId;
    emit firstDisplayedEventChanged();
}

void Room::setFirstDisplayedEvent(TimelineItem::index_t index)
{
    Q_ASSERT(isValidIndex(index));
    setFirstDisplayedEventId(findInTimeline(index)->event()->id());
}

QString Room::lastDisplayedEventId() const { return d->lastDisplayedEventId; }

Room::rev_iter_t Room::lastDisplayedMarker() const
{
    return findInTimeline(lastDisplayedEventId());
}

void Room::setLastDisplayedEventId(const QString& eventId)
{
    if (d->lastDisplayedEventId == eventId)
        return;

    const auto marker = findInTimeline(eventId);
    if (!eventId.isEmpty() && marker == historyEdge())
        qCWarning(MESSAGES)
            << eventId
            << "is marked as last displayed but doesn't seem to be loaded";

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
    return findInTimeline(lastReadReceipt(user->id()).eventId);
}

Room::rev_iter_t Room::readMarker() const { return fullyReadMarker(); }

QString Room::readMarkerEventId() const { return lastFullyReadEventId(); }

ReadReceipt Room::lastReadReceipt(const QString& userId) const
{
    return d->lastReadReceipts.value(userId);
}

ReadReceipt Room::lastLocalReadReceipt() const
{
    return d->lastReadReceipts.value(localUser()->id());
}

Room::rev_iter_t Room::localReadReceiptMarker() const
{
    return findInTimeline(lastLocalReadReceipt().eventId);
}

QString Room::lastFullyReadEventId() const { return d->fullyReadUntilEventId; }

Room::rev_iter_t Room::fullyReadMarker() const
{
    return findInTimeline(d->fullyReadUntilEventId);
}

QSet<QString> Room::userIdsAtEvent(const QString& eventId)
{
    return d->eventIdReadUsers.value(eventId);
}

QSet<User*> Room::usersAtEventId(const QString& eventId)
{
    const auto& userIds = d->eventIdReadUsers.value(eventId);
    QSet<User*> users;
    users.reserve(userIds.size());
    for (const auto& uId : userIds)
        users.insert(user(uId));
    return users;
}

qsizetype Room::notificationCount() const
{
    return d->unreadStats.notableCount;
}

void Room::resetNotificationCount()
{
    if (d->unreadStats.notableCount == 0)
        return;
    d->unreadStats.notableCount = 0;
    emit notificationCountChanged();
}

qsizetype Room::highlightCount() const { return d->serverHighlightCount; }

void Room::resetHighlightCount()
{
    if (d->serverHighlightCount == 0)
        return;
    d->serverHighlightCount = 0;
    emit highlightCountChanged();
}

void Room::switchVersion(QString newVersion)
{
    if (!successorId().isEmpty()) {
        Q_ASSERT(!successorId().isEmpty());
        emit upgradeFailed(tr("The room is already upgraded"));
    }
    if (auto* job = connection()->callApi<UpgradeRoomJob>(id(), newVersion))
        connect(job, &BaseJob::failure, this,
                [this, job] { emit upgradeFailed(job->errorString()); });
    else
        emit upgradeFailed(tr("Couldn't initiate upgrade"));
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

QStringList Room::tagNames() const { return d->tags.keys(); }

TagsMap Room::tags() const { return d->tags; }

TagRecord Room::tag(const QString& name) const { return d->tags.value(name); }

std::pair<bool, QString> validatedTag(QString name)
{
    if (name.isEmpty() || name.indexOf('.', 1) != -1)
        return { false, name };

    qCWarning(MAIN) << "The tag" << name
                    << "doesn't follow the CS API conventions";
    name.prepend("u.");
    qCWarning(MAIN) << "Using " << name << "instead";

    return { true, name };
}

void Room::addTag(const QString& name, const TagRecord& record)
{
    const auto& checkRes = validatedTag(name);
    if (d->tags.contains(name)
        || (checkRes.first && d->tags.contains(checkRes.second)))
        return;

    emit tagsAboutToChange();
    d->tags.insert(checkRes.second, record);
    emit tagsChanged();
    connection()->callApi<SetRoomTagJob>(localUser()->id(), id(),
                                         checkRes.second, record.order);
}

void Room::addTag(const QString& name, float order)
{
    addTag(name, TagRecord { order });
}

void Room::removeTag(const QString& name)
{
    if (d->tags.contains(name)) {
        emit tagsAboutToChange();
        d->tags.remove(name);
        emit tagsChanged();
        connection()->callApi<DeleteRoomTagJob>(localUser()->id(), id(), name);
    } else if (!name.startsWith("u."))
        removeTag("u." + name);
    else
        qCWarning(MAIN) << "Tag" << name << "on room" << objectName()
                       << "not found, nothing to remove";
}

void Room::setTags(TagsMap newTags, ActionScope applyOn)
{
    bool propagate = applyOn != ActionScope::ThisRoomOnly;
    auto joinStates =
        applyOn == ActionScope::WithinSameState ? joinState() :
        applyOn == ActionScope::OmitLeftState ? JoinState::Join|JoinState::Invite :
        JoinState::Join|JoinState::Invite|JoinState::Leave;
    if (propagate) {
        for (auto* r = this; (r = r->predecessor(joinStates));)
            r->setTags(newTags, ActionScope::ThisRoomOnly);
    }

    d->setTags(move(newTags));
    connection()->callApi<SetAccountDataPerRoomJob>(
        localUser()->id(), id(), TagEvent::matrixTypeId(),
        TagEvent(d->tags).contentJson());

    if (propagate) {
        for (auto* r = this; (r = r->successor(joinStates));)
            r->setTags(d->tags, ActionScope::ThisRoomOnly);
    }
}

void Room::Private::setTags(TagsMap&& newTags)
{
    emit q->tagsAboutToChange();
    const auto keys = newTags.keys();
    for (const auto& k : keys)
        if (const auto& [adjusted, adjustedTag] = validatedTag(k); adjusted) {
            if (newTags.contains(adjustedTag))
                newTags.remove(k);
            else
                newTags.insert(adjustedTag, newTags.take(k));
        }

    tags = move(newTags);
    qCDebug(STATE) << "Room" << q->objectName() << "is tagged with"
                   << q->tagNames().join(QStringLiteral(", "));
    emit q->tagsChanged();
}

bool Room::isFavourite() const { return d->tags.contains(FavouriteTag); }

bool Room::isLowPriority() const { return d->tags.contains(LowPriorityTag); }

bool Room::isServerNoticeRoom() const
{
    return d->tags.contains(ServerNoticeTag);
}

bool Room::isDirectChat() const { return connection()->isDirectChat(id()); }

QList<User*> Room::directChatUsers() const
{
    return connection()->directChatUsers(this);
}

QUrl Room::makeMediaUrl(const QString& eventId, const QUrl& mxcUrl) const
{
    auto url = connection()->makeMediaUrl(mxcUrl);
    QUrlQuery q(url.query());
    Q_ASSERT(q.hasQueryItem("user_id"));
    q.addQueryItem("room_id", id());
    q.addQueryItem("event_id", eventId);
    url.setQuery(q);
    return url;
}

QString safeFileName(QString rawName)
{
    return rawName.replace(QRegularExpression("[/\\<>|\"*?:]"), "_");
}

const RoomMessageEvent*
Room::Private::getEventWithFile(const QString& eventId) const
{
    auto evtIt = q->findInTimeline(eventId);
    if (evtIt != timeline.rend() && is<RoomMessageEvent>(**evtIt)) {
        auto* event = evtIt->viewAs<RoomMessageEvent>();
        if (event->hasFileContent())
            return event;
    }
    qCWarning(MAIN) << "No files to download in event" << eventId;
    return nullptr;
}

QString Room::Private::fileNameToDownload(const RoomMessageEvent* event) const
{
    Q_ASSERT(event && event->hasFileContent());
    const auto* fileInfo = event->content()->fileInfo();
    QString fileName;
    if (!fileInfo->originalName.isEmpty())
        fileName = QFileInfo(safeFileName(fileInfo->originalName)).fileName();
    else if (QUrl u { event->plainBody() }; u.isValid()) {
        qDebug(MAIN) << event->id()
                     << "has no file name supplied but the event body "
                        "looks like a URL - using the file name from it";
        fileName = u.fileName();
    }
    if (fileName.isEmpty())
        return safeFileName(fileInfo->mediaId()).replace('.', '-') % '.'
               % fileInfo->mimeType.preferredSuffix();

    if (QSysInfo::productType() == "windows") {
        if (const auto& suffixes = fileInfo->mimeType.suffixes();
            !suffixes.isEmpty()
            && std::none_of(suffixes.begin(), suffixes.end(),
                            [&fileName](const QString& s) {
                                return fileName.endsWith(s);
                            }))
            return fileName % '.' % fileInfo->mimeType.preferredSuffix();
    }
    return fileName;
}

QUrl Room::urlToThumbnail(const QString& eventId) const
{
    if (auto* event = d->getEventWithFile(eventId))
        if (event->hasThumbnail()) {
            auto* thumbnail = event->content()->thumbnailInfo();
            Q_ASSERT(thumbnail != nullptr);
            return connection()->getUrlForApi<MediaThumbnailJob>(
                thumbnail->url, thumbnail->imageSize);
        }
    qCDebug(MAIN) << "Event" << eventId << "has no thumbnail";
    return {};
}

QUrl Room::urlToDownload(const QString& eventId) const
{
    if (auto* event = d->getEventWithFile(eventId)) {
        auto* fileInfo = event->content()->fileInfo();
        Q_ASSERT(fileInfo != nullptr);
        return connection()->getUrlForApi<DownloadFileJob>(fileInfo->url);
    }
    return {};
}

QString Room::fileNameToDownload(const QString& eventId) const
{
    if (auto* event = d->getEventWithFile(eventId))
        return d->fileNameToDownload(event);
    return {};
}

FileTransferInfo Room::fileTransferInfo(const QString& id) const
{
    const auto infoIt = d->fileTransfers.constFind(id);
    if (infoIt == d->fileTransfers.cend())
        return {};

    // FIXME: Add lib tests to make sure FileTransferInfo::status stays
    // consistent with FileTransferInfo::job

    qint64 progress = infoIt->progress;
    qint64 total = infoIt->total;
    if (total > INT_MAX) {
        // JavaScript doesn't deal with 64-bit integers; scale down if necessary
        progress = llround(double(progress) / total * INT_MAX);
        total = INT_MAX;
    }

    return { infoIt->status,
             infoIt->isUpload,
             int(progress),
             int(total),
             QUrl::fromLocalFile(infoIt->localFileInfo.absolutePath()),
             QUrl::fromLocalFile(infoIt->localFileInfo.absoluteFilePath()) };
}

QUrl Room::fileSource(const QString& id) const
{
    auto url = urlToDownload(id);
    if (url.isValid())
        return url;

    // No urlToDownload means it's a pending or completed upload.
    auto infoIt = d->fileTransfers.constFind(id);
    if (infoIt != d->fileTransfers.cend())
        return QUrl::fromLocalFile(infoIt->localFileInfo.absoluteFilePath());

    qCWarning(MAIN) << "File source for identifier" << id << "not found";
    return {};
}

QString Room::prettyPrint(const QString& plainText) const
{
    return Quotient::prettyPrint(plainText);
}

QList<User*> Room::usersTyping() const { return d->usersTyping; }

QList<User*> Room::membersLeft() const { return d->membersLeft; }

QList<User*> Room::users() const { return d->membersMap.values(); }

QStringList Room::memberNames() const
{
    return safeMemberNames();
}

QStringList Room::safeMemberNames() const
{
    QStringList res;
    res.reserve(d->membersMap.size());
    for (auto u: std::as_const(d->membersMap))
        res.append(safeMemberName(u->id()));

    return res;
}

QStringList Room::htmlSafeMemberNames() const
{
    QStringList res;
    res.reserve(d->membersMap.size());
    for (auto u: std::as_const(d->membersMap))
        res.append(htmlSafeMemberName(u->id()));

    return res;
}

int Room::timelineSize() const { return int(d->timeline.size()); }

bool Room::usesEncryption() const
{
    return !d->getCurrentState<EncryptionEvent>()->algorithm().isEmpty();
}

const StateEventBase* Room::getCurrentState(const QString& evtType,
                                            const QString& stateKey) const
{
    return d->getCurrentState({ evtType, stateKey });
}

const QVector<const StateEventBase*>
Room::stateEventsOfType(const QString& evtType) const
{
    return d->stateEventsOfType(evtType);
}

const QHash<StateEventKey, const StateEventBase*>& Room::currentState() const
{
    return d->currentState;
}

RoomEventPtr Room::decryptMessage(const EncryptedEvent& encryptedEvent)
{
#ifndef Quotient_E2EE_ENABLED
    Q_UNUSED(encryptedEvent)
    qCWarning(E2EE) << "End-to-end encryption (E2EE) support is turned off.";
    return {};
#else // Quotient_E2EE_ENABLED
    if (encryptedEvent.algorithm() == MegolmV1AesSha2AlgoKey) {
        QString decrypted = d->groupSessionDecryptMessage(
            encryptedEvent.ciphertext(), encryptedEvent.senderKey(),
            encryptedEvent.sessionId(), encryptedEvent.id(),
            encryptedEvent.originTimestamp());
        if (decrypted.isEmpty()) {
            return {};
        }
        return makeEvent<RoomMessageEvent>(
            QJsonDocument::fromJson(decrypted.toUtf8()).object());
    }
    qCDebug(E2EE) << "Algorithm of the encrypted event with id"
                  << encryptedEvent.id() << "is not for the current device";
    return {};
#endif // Quotient_E2EE_ENABLED
}

void Room::handleRoomKeyEvent(const RoomKeyEvent& roomKeyEvent,
                              const QString& senderKey)
{
#ifndef Quotient_E2EE_ENABLED
    Q_UNUSED(roomKeyEvent)
    Q_UNUSED(senderKey)
    qCWarning(E2EE) << "End-to-end encryption (E2EE) support is turned off.";
#else // Quotient_E2EE_ENABLED
    if (roomKeyEvent.algorithm() != MegolmV1AesSha2AlgoKey) {
        qCWarning(E2EE) << "Ignoring unsupported algorithm"
                        << roomKeyEvent.algorithm() << "in m.room_key event";
    }
    if (d->addInboundGroupSession(senderKey, roomKeyEvent.sessionId(),
                                  roomKeyEvent.sessionKey())) {
        qCDebug(E2EE) << "added new inboundGroupSession:"
                      << d->groupSessions.count();
    }
#endif // Quotient_E2EE_ENABLED
}

int Room::joinedCount() const
{
    return d->summary.joinedMemberCount.value_or(d->membersMap.size());
}

int Room::invitedCount() const
{
    // TODO: Store invited users in Room too
    Q_ASSERT(d->summary.invitedMemberCount.has_value());
    return d->summary.invitedMemberCount.value_or(0);
}

int Room::totalMemberCount() const { return joinedCount() + invitedCount(); }

GetRoomEventsJob* Room::eventsHistoryJob() const { return d->eventsHistoryJob; }

Room::Changes Room::Private::setSummary(RoomSummary&& newSummary)
{
    if (!summary.merge(newSummary))
        return Change::None;
    qCDebug(STATE).nospace().noquote()
        << "Updated room summary for " << q->objectName() << ": " << summary;
    return Change::Summary;
}

void Room::Private::insertMemberIntoMap(User* u)
{
    const auto maybeUserName =
        getCurrentState<RoomMemberEvent>(u->id())->newDisplayName();
    if (!maybeUserName)
        qCWarning(MEMBERS) << "insertMemberIntoMap():" << u->id()
                           << "has no name (even empty)";
    const auto userName = maybeUserName.value_or(QString());
    const auto namesakes = membersMap.values(userName);
    qCDebug(MEMBERS) << "insertMemberIntoMap(), user" << u->id()
                     << "with name" << userName << '-'
                     << namesakes.size() << "namesake(s) found";

    // Callers should make sure they are not adding an existing user once more
    Q_ASSERT(!namesakes.contains(u));
    if (namesakes.contains(u)) { // Release version whines but continues
        qCCritical(MEMBERS) << "Trying to add a user" << u->id() << "to room"
                            << q->objectName() << "but that's already in it";
        return;
    }

    // If there is exactly one namesake of the added user, signal member
    // renaming for that other one because the two should be disambiguated now
    if (namesakes.size() == 1)
        emit q->memberAboutToRename(namesakes.front(),
                                    namesakes.front()->fullName(q));
    membersMap.insert(userName, u);
    if (namesakes.size() == 1)
        emit q->memberRenamed(namesakes.front());
}

void Room::Private::removeMemberFromMap(User* u)
{
    const auto userName =
        getCurrentState<RoomMemberEvent>(
                u->id())->newDisplayName().value_or(QString());

    qCDebug(MEMBERS) << "removeMemberFromMap(), username" << userName
                     << "for user" << u->id();
    User* namesake = nullptr;
    auto namesakes = membersMap.values(userName);
    // If there was one namesake besides the removed user, signal member
    // renaming for it because it doesn't need to be disambiguated any more.
    if (namesakes.size() == 2) {
        namesake =
            namesakes.front() == u ? namesakes.back() : namesakes.front();
        Q_ASSERT_X(namesake != u, __FUNCTION__, "Room members list is broken");
        emit q->memberAboutToRename(namesake, userName);
    }
    if (membersMap.remove(userName, u) == 0) {
        qCDebug(MEMBERS) << "No entries removed; checking the whole list";
        // Unless at the stage of initial filling, this no removed entries
        // is suspicious; double-check that this user is not found in
        // the whole map, and stop (for debug builds) or shout in the logs
        // (for release builds) if there's one. That search is O(n), which
        // may come rather expensive for larger rooms.
        QElapsedTimer et;
        auto it = std::find(membersMap.cbegin(), membersMap.cend(), u);
        if (et.nsecsElapsed() > profilerMinNsecs() / 10)
            qCDebug(MEMBERS) << "...done in" << et;
        if (it != membersMap.cend()) {
            // The assert (still) does more harm than good, it seems
//            Q_ASSERT_X(false, __FUNCTION__,
//                       "Mismatched name in the room members list");
            qCCritical(MEMBERS) << "Mismatched name in the room members list;"
                                   " avoiding the list corruption";
            membersMap.remove(it.key(), u);
        }
    }
    if (namesake)
        emit q->memberRenamed(namesake);
}

inline auto makeErrorStr(const Event& e, QByteArray msg)
{
    return msg.append("; event dump follows:\n")
        .append(QJsonDocument(e.fullJson()).toJson());
}

Room::Timeline::size_type
Room::Private::moveEventsToTimeline(RoomEventsRange events,
                                    EventsPlacement placement)
{
    Q_ASSERT(!events.empty());
    // Historical messages arrive in newest-to-oldest order, so the process for
    // them is almost symmetric to the one for new messages. New messages get
    // appended from index 0; old messages go backwards from index -1.
    auto index = timeline.empty()
                     ? -((placement + 1) / 2) /* 1 -> -1; -1 -> 0 */
                     : placement == Older ? timeline.front().index()
                                          : timeline.back().index();
    auto baseIndex = index;
    for (auto&& e : events) {
        const auto eId = e->id();
        Q_ASSERT_X(e, __FUNCTION__, "Attempt to add nullptr to timeline");
        Q_ASSERT_X(
            !eId.isEmpty(), __FUNCTION__,
            makeErrorStr(*e, "Event with empty id cannot be in the timeline"));
        Q_ASSERT_X(
            !eventsIndex.contains(eId), __FUNCTION__,
            makeErrorStr(*e, "Event is already in the timeline; "
                             "incoming events were not properly deduplicated"));
        const auto& ti = placement == Older
                             ? timeline.emplace_front(move(e), --index)
                             : timeline.emplace_back(move(e), ++index);
        eventsIndex.insert(eId, index);
        if (auto n = q->checkForNotifications(ti); n.type != Notification::None)
            notifications.insert(e->id(), n);
        Q_ASSERT(q->findInTimeline(eId)->event()->id() == eId);
    }
    const auto insertedSize = (index - baseIndex) * placement;
    Q_ASSERT(insertedSize == int(events.size()));
    return Timeline::size_type(insertedSize);
}

QString Room::memberName(const QString& mxId) const
{
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    const auto rme = getCurrentState<RoomMemberEvent>(mxId);
    return rme->newDisplayName() ? *rme->newDisplayName()
        : rme->prevContent() ? rme->prevContent()->displayName.value_or(QString())
        : QString();
}

QString Room::roomMembername(const User* u) const
{
    Q_ASSERT(u != nullptr);
    return disambiguatedMemberName(u->id());
}

QString Room::roomMembername(const QString& userId) const
{
    return disambiguatedMemberName(userId);
}

inline QString makeFullUserName(const QString& displayName, const QString& mxId)
{
    return displayName % " (" % mxId % ')';
}

QString Room::disambiguatedMemberName(const QString& mxId) const
{
    // See the CS spec, section 11.2.2.3

    const auto username = memberName(mxId);
    if (username.isEmpty())
        return mxId;

    auto namesakesIt = qAsConst(d->membersMap).find(username);

    // We expect a user to be a member of the room - but technically it is
    // possible to invoke this function even for non-members. In such case
    // we return the full name, just in case.
    if (namesakesIt == d->membersMap.cend())
        return makeFullUserName(username, mxId);

    auto nextUserIt = namesakesIt;
    if (++nextUserIt == d->membersMap.cend() || nextUserIt.key() != username)
        return username; // No disambiguation necessary

    return makeFullUserName(username, mxId); // Disambiguate fully
}

QString Room::safeMemberName(const QString& userId) const
{
    return sanitized(disambiguatedMemberName(userId));
}

QString Room::htmlSafeMemberName(const QString& userId) const
{
    return safeMemberName(userId).toHtmlEscaped();
}

QUrl Room::memberAvatarUrl(const QString &mxId) const
{
    // See https://github.com/matrix-org/matrix-doc/issues/1375
    const auto rme = getCurrentState<RoomMemberEvent>(mxId);
    return rme->newAvatarUrl() ? *rme->newAvatarUrl()
        : rme->prevContent() ? rme->prevContent()->avatarUrl.value_or(QUrl())
        : QUrl();
}

Room::Changes Room::Private::updateStatsFromSyncData(const SyncRoomData& data,
                                                     bool fromCache)
{
    Changes changes {};
    if (fromCache) {
        // Initial load of cached statistics
        partiallyReadStats =
            EventStats::fromCachedCounters(data.partiallyReadCount);
        unreadStats = EventStats::fromCachedCounters(data.unreadCount,
                                                     data.highlightCount);
        // Migrate from lib 0.6: -1 in the old unread counter overrides 0
        // (which loads to an estimate) in notification_count. Next caching will
        // save -1 in both places, completing the migration.
        if (data.unreadCount == 0 && data.partiallyReadCount == -1)
            unreadStats.isEstimate = false;
        changes |= Change::PartiallyReadStats | Change::UnreadStats;
        qCDebug(MESSAGES) << "Loaded" << q->objectName()
                          << "event statistics from cache:" << partiallyReadStats
                          << "since m.fully_read," << unreadStats
                          << "since m.read";
    } else if (timeline.empty()) {
        // In absence of actual events use statistics from the homeserver
        if (merge(unreadStats.notableCount, data.unreadCount))
            changes |= Change::PartiallyReadStats;
        if (merge(unreadStats.highlightCount, data.highlightCount))
            changes |= Change::UnreadStats;
        unreadStats.isEstimate = !data.unreadCount.has_value()
                                 || *data.unreadCount > 0;
        qCDebug(MESSAGES)
            << "Using server-side unread event statistics while the"
            << q->objectName() << "timeline is empty:" << unreadStats;
    }
    bool correctedStats = false;
    if (unreadStats.highlightCount > partiallyReadStats.highlightCount) {
        correctedStats = true;
        partiallyReadStats.highlightCount = unreadStats.highlightCount;
        partiallyReadStats.isEstimate |= unreadStats.isEstimate;
    }
    if (unreadStats.notableCount > partiallyReadStats.notableCount) {
        correctedStats = true;
        partiallyReadStats.notableCount = unreadStats.notableCount;
        partiallyReadStats.isEstimate |= unreadStats.isEstimate;
    }
    if (!unreadStats.isEstimate && partiallyReadStats.isEstimate) {
        correctedStats = true;
        partiallyReadStats.isEstimate = true;
    }
    if (correctedStats)
        qCDebug(MESSAGES) << "Partially read event statistics in"
                          << q->objectName() << "were adjusted to"
                          << partiallyReadStats
                          << "to be consistent with the m.read receipt";
    Q_ASSERT(partiallyReadStats.isValidFor(q, q->fullyReadMarker()));
    Q_ASSERT(unreadStats.isValidFor(q, q->localReadReceiptMarker()));

    // TODO: Once the library learns to count highlights, drop
    // serverHighlightCount and only use the server-side counter when
    // the timeline is empty (see the code above).
    if (merge(serverHighlightCount, data.highlightCount)) {
        qCDebug(MESSAGES) << "Updated highlights number in" << q->objectName()
                          << "to" << serverHighlightCount;
        changes |= Change::Highlights;
    }
    return changes;
}

void Room::updateData(SyncRoomData&& data, bool fromCache)
{
    if (d->prevBatch.isEmpty())
        d->prevBatch = data.timelinePrevBatch;
    setJoinState(data.joinState);

    Changes roomChanges {};
    // The order of calculation is important - don't merge the lines!
    roomChanges |= d->updateStateFrom(data.state);
    roomChanges |= d->setSummary(move(data.summary));
    roomChanges |= d->addNewMessageEvents(move(data.timeline));

    for (auto&& ephemeralEvent : data.ephemeral)
        roomChanges |= processEphemeralEvent(move(ephemeralEvent));

    for (auto&& event : data.accountData)
        roomChanges |= processAccountDataEvent(move(event));

    roomChanges |= d->updateStatsFromSyncData(data, fromCache);

    if (roomChanges & Change::Topic)
        emit topicChanged();

    if (roomChanges & (Change::Name | Change::Aliases))
        emit namesChanged(this);

    d->postprocessChanges(roomChanges, !fromCache);
}

void Room::Private::postprocessChanges(Changes changes, bool saveState)
{
    if (!changes)
        return;

    if (changes & Change::Members)
        emit q->memberListChanged();

    if (changes
        & (Change::Name | Change::Aliases | Change::Members | Change::Summary))
        updateDisplayname();

    if (changes & Change::PartiallyReadStats) {
        emit q->unreadMessagesChanged(q); // TODO: remove in 0.8
        emit q->partiallyReadStatsChanged();
    }

    if (changes & Change::UnreadStats)
        emit q->unreadStatsChanged();

    if (changes & Change::Highlights)
        emit q->highlightCountChanged();

    qCDebug(MAIN) << terse << changes << "= hex" <<
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::
#endif
            hex << uint(changes) << "in" << q->objectName();
    emit q->changed(changes);
    if (saveState)
        connection->saveRoomState(q);
}

RoomEvent* Room::Private::addAsPending(RoomEventPtr&& event)
{
    if (event->transactionId().isEmpty())
        event->setTransactionId(connection->generateTxnId());
    if (event->roomId().isEmpty())
        event->setRoomId(id);
    if (event->senderId().isEmpty())
        event->setSender(connection->userId());
    auto* pEvent = rawPtr(event);
    emit q->pendingEventAboutToAdd(pEvent);
    unsyncedEvents.emplace_back(move(event));
    emit q->pendingEventAdded();
    return pEvent;
}

QString Room::Private::sendEvent(RoomEventPtr&& event)
{
    if (q->usesEncryption()) {
        qCCritical(MAIN) << "Room" << q->objectName()
                         << "enforces encryption; sending encrypted messages "
                            "is not supported yet";
    }
    if (q->successorId().isEmpty())
        return doSendEvent(addAsPending(std::move(event)));

    qCWarning(MAIN) << q << "has been upgraded, event won't be sent";
    return {};
}

QString Room::Private::doSendEvent(const RoomEvent* pEvent)
{
    const auto txnId = pEvent->transactionId();
    // TODO, #133: Enqueue the job rather than immediately trigger it.
    if (auto call =
            connection->callApi<SendMessageJob>(BackgroundRequest, id,
                                                pEvent->matrixType(), txnId,
                                                pEvent->contentJson())) {
        Room::connect(call, &BaseJob::sentRequest, q, [this, txnId] {
            auto it = q->findPendingEvent(txnId);
            if (it == unsyncedEvents.end()) {
                qCWarning(EVENTS) << "Pending event for transaction" << txnId
                                 << "not found - got synced so soon?";
                return;
            }
            it->setDeparted();
            qCDebug(EVENTS) << "Event txn" << txnId << "has departed";
            emit q->pendingEventChanged(int(it - unsyncedEvents.begin()));
        });
        Room::connect(call, &BaseJob::failure, q,
                      std::bind(&Room::Private::onEventSendingFailure, this,
                                txnId, call));
        Room::connect(call, &BaseJob::success, q, [this, call, txnId] {
            auto it = q->findPendingEvent(txnId);
            if (it != unsyncedEvents.end()) {
                if (it->deliveryStatus() != EventStatus::ReachedServer) {
                    it->setReachedServer(call->eventId());
                    emit q->pendingEventChanged(int(it - unsyncedEvents.begin()));
                }
            } else
                qCDebug(EVENTS) << "Pending event for transaction" << txnId
                               << "already merged";

            emit q->messageSent(txnId, call->eventId());
        });
    } else
        onEventSendingFailure(txnId);
    return txnId;
}

void Room::Private::onEventSendingFailure(const QString& txnId, BaseJob* call)
{
    auto it = q->findPendingEvent(txnId);
    if (it == unsyncedEvents.end()) {
        qCritical(EVENTS) << "Pending event for transaction" << txnId
                          << "could not be sent";
        return;
    }
    it->setSendingFailed(call ? call->statusCaption() % ": " % call->errorString()
                              : tr("The call could not be started"));
    emit q->pendingEventChanged(int(it - unsyncedEvents.begin()));
}

QString Room::retryMessage(const QString& txnId)
{
    const auto it = findPendingEvent(txnId);
    Q_ASSERT(it != d->unsyncedEvents.end());
    qCDebug(EVENTS) << "Retrying transaction" << txnId;
    const auto& transferIt = d->fileTransfers.constFind(txnId);
    if (transferIt != d->fileTransfers.cend()) {
        Q_ASSERT(transferIt->isUpload);
        if (transferIt->status == FileTransferInfo::Completed) {
            qCDebug(MESSAGES)
                << "File for transaction" << txnId
                << "has already been uploaded, bypassing re-upload";
        } else {
            if (isJobPending(transferIt->job)) {
                qCDebug(MESSAGES) << "Abandoning the upload job for transaction"
                                  << txnId << "and starting again";
                transferIt->job->abandon();
                emit fileTransferFailed(txnId,
                                        tr("File upload will be retried"));
            }
            uploadFile(txnId, QUrl::fromLocalFile(
                                  transferIt->localFileInfo.absoluteFilePath()));
            // FIXME: Content type is no more passed here but it should
        }
    }
    if (it->deliveryStatus() == EventStatus::ReachedServer) {
        qCWarning(MAIN)
            << "The previous attempt has reached the server; two"
               " events are likely to be in the timeline after retry";
    }
    it->resetStatus();
    emit pendingEventChanged(int(it - d->unsyncedEvents.begin()));
    return d->doSendEvent(it->event());
}

// Lambda defers actual tr() invocation to the moment when translations are
// initialised
const auto FileTransferCancelledMsg = [] {
    return Room::tr("File transfer cancelled");
};

void Room::discardMessage(const QString& txnId)
{
    auto it = std::find_if(d->unsyncedEvents.begin(), d->unsyncedEvents.end(),
                           [txnId](const auto& evt) {
                               return evt->transactionId() == txnId;
                           });
    Q_ASSERT(it != d->unsyncedEvents.end());
    qCDebug(EVENTS) << "Discarding transaction" << txnId;
    const auto& transferIt = d->fileTransfers.find(txnId);
    if (transferIt != d->fileTransfers.end()) {
        Q_ASSERT(transferIt->isUpload);
        if (isJobPending(transferIt->job)) {
            transferIt->status = FileTransferInfo::Cancelled;
            transferIt->job->abandon();
            emit fileTransferFailed(txnId, FileTransferCancelledMsg());
        } else if (transferIt->status == FileTransferInfo::Completed) {
            qCWarning(MAIN)
                << "File for transaction" << txnId
                << "has been uploaded but the message was discarded";
        }
    }
    emit pendingEventAboutToDiscard(int(it - d->unsyncedEvents.begin()));
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
    return d->sendEvent<RoomMessageEvent>(
        plainText, type,
        new EventContent::TextContent(html, QStringLiteral("text/html")));
}

QString Room::postHtmlText(const QString& plainText, const QString& html)
{
    return postHtmlMessage(plainText, html);
}

QString Room::postReaction(const QString& eventId, const QString& key)
{
    return d->sendEvent<ReactionEvent>(EventRelation::annotate(eventId, key));
}

QString Room::Private::doPostFile(RoomEventPtr&& msgEvent, const QUrl& localUrl)
{
    const auto txnId = addAsPending(move(msgEvent))->transactionId();
    // Remote URL will only be known after upload; fill in the local path
    // to enable the preview while the event is pending.
    q->uploadFile(txnId, localUrl);
    // Below, the upload job is used as a context object to clean up connections
    const auto& transferJob = fileTransfers.value(txnId).job;
    connect(q, &Room::fileTransferCompleted, transferJob,
            [this, txnId](const QString& tId, const QUrl&, const QUrl& mxcUri) {
                if (tId != txnId)
                    return;

                const auto it = q->findPendingEvent(txnId);
                if (it != unsyncedEvents.end()) {
                    it->setFileUploaded(mxcUri);
                    emit q->pendingEventChanged(
                        int(it - unsyncedEvents.begin()));
                    doSendEvent(it->get());
                } else {
                    // Normally in this situation we should instruct
                    // the media server to delete the file; alas, there's no
                    // API specced for that.
                    qCWarning(MAIN) << "File uploaded to" << mxcUri
                                    << "but the event referring to it was "
                                       "cancelled";
                }
            });
    connect(q, &Room::fileTransferFailed, transferJob,
            [this, txnId](const QString& tId) {
                if (tId != txnId)
                    return;

                const auto it = q->findPendingEvent(txnId);
                if (it == unsyncedEvents.end())
                    return;

                const auto idx = int(it - unsyncedEvents.begin());
                emit q->pendingEventAboutToDiscard(idx);
                // See #286 on why `it` may not be valid here.
                unsyncedEvents.erase(unsyncedEvents.begin() + idx);
                emit q->pendingEventDiscarded();
            });

    return txnId;
}

QString Room::postFile(const QString& plainText,
                       EventContent::TypedBase* content)
{
    Q_ASSERT(content != nullptr && content->fileInfo() != nullptr);
    const auto* const fileInfo = content->fileInfo();
    Q_ASSERT(fileInfo != nullptr);
    QFileInfo localFile { fileInfo->url.toLocalFile() };
    Q_ASSERT(localFile.isFile());

    return d->doPostFile(
        makeEvent<RoomMessageEvent>(
            plainText, RoomMessageEvent::rawMsgTypeForFile(localFile), content),
        fileInfo->url);
}

#if QT_VERSION_MAJOR < 6
QString Room::postFile(const QString& plainText, const QUrl& localPath,
                       bool asGenericFile)
{
    QFileInfo localFile { localPath.toLocalFile() };
    Q_ASSERT(localFile.isFile());
    return d->doPostFile(makeEvent<RoomMessageEvent>(plainText, localFile,
                                                     asGenericFile),
                         localPath);
}
#endif

QString Room::postEvent(RoomEvent* event)
{
    return d->sendEvent(RoomEventPtr(event));
}

QString Room::postJson(const QString& matrixType,
                       const QJsonObject& eventContent)
{
    return d->sendEvent(loadEvent<RoomEvent>(matrixType, eventContent));
}

SetRoomStateWithKeyJob* Room::setState(const StateEventBase& evt) const
{
    return d->requestSetState(evt);
}

void Room::setName(const QString& newName)
{
    d->requestSetState<RoomNameEvent>(newName);
}

void Room::setCanonicalAlias(const QString& newAlias)
{
    d->requestSetState<RoomCanonicalAliasEvent>(newAlias, altAliases());
}

void Room::setLocalAliases(const QStringList& aliases)
{
    d->requestSetState<RoomCanonicalAliasEvent>(canonicalAlias(), aliases);
}

void Room::setTopic(const QString& newTopic)
{
    d->requestSetState<RoomTopicEvent>(newTopic);
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

bool Room::supportsCalls() const { return joinedCount() == 2; }

void Room::checkVersion()
{
    const auto defaultVersion = connection()->defaultRoomVersion();
    const auto stableVersions = connection()->stableRoomVersions();
    Q_ASSERT(!defaultVersion.isEmpty());
    // This method is only called after the base state has been loaded
    // or the server capabilities have been loaded.
    emit stabilityUpdated(defaultVersion, stableVersions);
    if (!stableVersions.contains(version())) {
        qCDebug(STATE) << this << "version is" << version()
                       << "which the server doesn't count as stable";
        if (canSwitchVersions())
            qCDebug(STATE)
                << "The current user has enough privileges to fix it";
    }
}

void Room::inviteCall(const QString& callId, const int lifetime,
                      const QString& sdp)
{
    Q_ASSERT(supportsCalls());
    d->sendEvent<CallInviteEvent>(callId, lifetime, sdp);
}

void Room::sendCallCandidates(const QString& callId,
                              const QJsonArray& candidates)
{
    Q_ASSERT(supportsCalls());
    d->sendEvent<CallCandidatesEvent>(callId, candidates);
}

void Room::answerCall(const QString& callId, const int lifetime,
                      const QString& sdp)
{
    Q_ASSERT(supportsCalls());
    d->sendEvent<CallAnswerEvent>(callId, lifetime, sdp);
}

void Room::answerCall(const QString& callId, const QString& sdp)
{
    Q_ASSERT(supportsCalls());
    d->sendEvent<CallAnswerEvent>(callId, sdp);
}

void Room::hangupCall(const QString& callId)
{
    Q_ASSERT(supportsCalls());
    d->sendEvent<CallHangupEvent>(callId);
}

void Room::getPreviousContent(int limit, const QString &filter) { d->getPreviousContent(limit, filter); }

void Room::Private::getPreviousContent(int limit, const QString &filter)
{
    if (isJobPending(eventsHistoryJob))
        return;

    eventsHistoryJob =
        connection->callApi<GetRoomEventsJob>(id, prevBatch, "b", "", limit, filter);
    emit q->eventsHistoryJobChanged();
    connect(eventsHistoryJob, &BaseJob::success, q, [this] {
        prevBatch = eventsHistoryJob->end();
        addHistoricalMessageEvents(eventsHistoryJob->chunk());
    });
    connect(eventsHistoryJob, &QObject::destroyed, q,
            &Room::eventsHistoryJobChanged);
}

void Room::inviteToRoom(const QString& memberId)
{
    connection()->callApi<InviteUserJob>(id(), memberId);
}

LeaveRoomJob* Room::leaveRoom()
{
    // FIXME, #63: It should be RoomManager, not Connection
    return connection()->leaveRoom(this);
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
    connection()->callApi<RedactEventJob>(id(), QUrl::toPercentEncoding(eventId),
                                          connection()->generateTxnId(), reason);
}

void Room::uploadFile(const QString& id, const QUrl& localFilename,
                      const QString& overrideContentType)
{
    Q_ASSERT_X(localFilename.isLocalFile(), __FUNCTION__,
               "localFilename should point at a local file");
    auto fileName = localFilename.toLocalFile();
    auto job = connection()->uploadFile(fileName, overrideContentType);
    if (isJobPending(job)) {
        d->fileTransfers[id] = { job, fileName, true };
        connect(job, &BaseJob::uploadProgress, this,
                [this, id](qint64 sent, qint64 total) {
                    d->fileTransfers[id].update(sent, total);
                    emit fileTransferProgress(id, sent, total);
                });
        connect(job, &BaseJob::success, this, [this, id, localFilename, job] {
            d->fileTransfers[id].status = FileTransferInfo::Completed;
            emit fileTransferCompleted(id, localFilename, QUrl(job->contentUri()));
        });
        connect(job, &BaseJob::failure, this,
                std::bind(&Private::failedTransfer, d, id, job->errorString()));
        emit newFileTransfer(id, localFilename);
    } else
        d->failedTransfer(id);
}

void Room::downloadFile(const QString& eventId, const QUrl& localFilename)
{
    if (auto ongoingTransfer = d->fileTransfers.constFind(eventId);
        ongoingTransfer != d->fileTransfers.cend()
        && ongoingTransfer->status == FileTransferInfo::Started) {
        qCWarning(MAIN) << "Transfer for" << eventId
                        << "is ongoing; download won't start";
        return;
    }

    Q_ASSERT_X(localFilename.isEmpty() || localFilename.isLocalFile(),
               __FUNCTION__, "localFilename should point at a local file");
    const auto* event = d->getEventWithFile(eventId);
    if (!event) {
        qCCritical(MAIN)
            << eventId << "is not in the local timeline or has no file content";
        Q_ASSERT(false);
        return;
    }
    const auto* const fileInfo = event->content()->fileInfo();
    if (!fileInfo->isValid()) {
        qCWarning(MAIN) << "Event" << eventId
                        << "has an empty or malformed mxc URL; won't download";
        return;
    }
    const auto fileUrl = fileInfo->url;
    auto filePath = localFilename.toLocalFile();
    if (filePath.isEmpty()) { // Setup default file path
        filePath =
            fileInfo->url.path().mid(1) % '_' % d->fileNameToDownload(event);

        if (filePath.size() > 200) // If too long, elide in the middle
            filePath.replace(128, filePath.size() - 192, "---");

        filePath = QDir::tempPath() % '/' % filePath;
        qDebug(MAIN) << "File path:" << filePath;
    }
    auto job = connection()->downloadFile(fileUrl, filePath);
    if (isJobPending(job)) {
        // If there was a previous transfer (completed or failed), overwrite it.
        d->fileTransfers[eventId] = { job, job->targetFileName() };
        connect(job, &BaseJob::downloadProgress, this,
                [this, eventId](qint64 received, qint64 total) {
                    d->fileTransfers[eventId].update(received, total);
                    emit fileTransferProgress(eventId, received, total);
                });
        connect(job, &BaseJob::success, this, [this, eventId, fileUrl, job] {
            d->fileTransfers[eventId].status = FileTransferInfo::Completed;
            emit fileTransferCompleted(
                eventId, fileUrl, QUrl::fromLocalFile(job->targetFileName()));
        });
        connect(job, &BaseJob::failure, this,
                std::bind(&Private::failedTransfer, d, eventId,
                          job->errorString()));
    } else
        d->failedTransfer(eventId);
}

void Room::cancelFileTransfer(const QString& id)
{
    const auto it = d->fileTransfers.find(id);
    if (it == d->fileTransfers.end()) {
        qCWarning(MAIN) << "No information on file transfer" << id << "in room"
                        << d->id;
        return;
    }
    if (isJobPending(it->job))
        it->job->abandon();
    it->status = FileTransferInfo::Cancelled;
    emit fileTransferFailed(id, FileTransferCancelledMsg());
}

void Room::Private::dropDuplicateEvents(RoomEvents& events) const
{
    if (events.empty())
        return;

    // Multiple-remove (by different criteria), single-erase
    // 1. Check for duplicates against the timeline.
    auto dupsBegin =
        remove_if(events.begin(), events.end(), [&](const RoomEventPtr& e) {
            return eventsIndex.contains(e->id());
        });

    // 2. Check for duplicates within the batch if there are still events.
    for (auto eIt = events.begin(); distance(eIt, dupsBegin) > 1; ++eIt)
        dupsBegin = remove_if(eIt + 1, dupsBegin, [&](const RoomEventPtr& e) {
            return e->id() == (*eIt)->id();
        });
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
    auto originalJson = target.fullJson();
    // clang-format off
    static const QStringList keepKeys {
        EventIdKey, TypeKey, RoomIdKey, SenderKey, StateKeyKey,
        QStringLiteral("hashes"), QStringLiteral("signatures"),
        QStringLiteral("depth"), QStringLiteral("prev_events"),
        QStringLiteral("prev_state"), QStringLiteral("auth_events"),
        QStringLiteral("origin"), QStringLiteral("origin_server_ts"),
        QStringLiteral("membership") };
    // clang-format on

    static const std::pair<event_type_t, QStringList> keepContentKeysMap[] {
        { RoomMemberEvent::typeId(), { QStringLiteral("membership") } },
        { RoomCreateEvent::typeId(), { QStringLiteral("creator") } },
        { RoomPowerLevelsEvent::typeId(),
          { QStringLiteral("ban"), QStringLiteral("events"),
            QStringLiteral("events_default"), QStringLiteral("kick"),
            QStringLiteral("redact"), QStringLiteral("state_default"),
            QStringLiteral("users"), QStringLiteral("users_default") } }
        //        , { RoomJoinRules::typeId(), { QStringLiteral("join_rule") } }
        //        , { RoomHistoryVisibility::typeId(),
        //                { QStringLiteral("history_visibility") } }
    };
    for (auto it = originalJson.begin(); it != originalJson.end();) {
        if (!keepKeys.contains(it.key()))
            it = originalJson.erase(it); // TODO: shred the value
        else
            ++it;
    }
    auto keepContentKeys =
        find_if(begin(keepContentKeysMap), end(keepContentKeysMap),
                [&target](const auto& t) { return target.type() == t.first; });
    if (keepContentKeys == end(keepContentKeysMap)) {
        originalJson.remove(ContentKeyL);
        originalJson.remove(PrevContentKeyL);
    } else {
        auto content = originalJson.take(ContentKeyL).toObject();
        for (auto it = content.begin(); it != content.end();) {
            if (!keepContentKeys->second.contains(it.key()))
                it = content.erase(it);
            else
                ++it;
        }
        originalJson.insert(ContentKey, content);
    }
    auto unsignedData = originalJson.take(UnsignedKeyL).toObject();
    unsignedData[RedactedCauseKeyL] = redaction.fullJson();
    originalJson.insert(QStringLiteral("unsigned"), unsignedData);

    return loadEvent<RoomEvent>(originalJson);
}

bool Room::Private::processRedaction(const RedactionEvent& redaction)
{
    // Can't use findInTimeline because it returns a const iterator, and
    // we need to change the underlying TimelineItem.
    const auto pIdx = eventsIndex.constFind(redaction.redactedEvent());
    if (pIdx == eventsIndex.cend())
        return false;

    Q_ASSERT(q->isValidIndex(*pIdx));

    auto& ti = timeline[Timeline::size_type(*pIdx - q->minTimelineIndex())];
    if (ti->isRedacted() && ti->redactedBecause()->id() == redaction.id()) {
        qCDebug(EVENTS) << "Redaction" << redaction.id() << "of event"
                        << ti->id() << "already done, skipping";
        return true;
    }

    // Make a new event from the redacted JSON and put it in the timeline
    // instead of the redacted one. oldEvent will be deleted on return.
    auto oldEvent = ti.replaceEvent(makeRedacted(*ti, redaction));
    qCDebug(EVENTS) << "Redacted" << oldEvent->id() << "with" << redaction.id();
    if (oldEvent->isStateEvent()) {
        const StateEventKey evtKey { oldEvent->matrixType(),
                                     oldEvent->stateKey() };
        Q_ASSERT(currentState.contains(evtKey));
        if (currentState.value(evtKey) == oldEvent.get()) {
            Q_ASSERT(ti.index() >= 0); // Historical states can't be in
                                       // currentState
            qCDebug(STATE).nospace()
                << "Redacting state " << oldEvent->matrixType() << "/"
                << oldEvent->stateKey();
            // Retarget the current state to the newly made event.
            if (q->processStateEvent(*ti))
                emit q->namesChanged(q);
            updateDisplayname();
        }
    }
    if (const auto* reaction = eventCast<ReactionEvent>(oldEvent)) {
        const auto& targetEvtId = reaction->relation().eventId;
        const auto lookupKey =
            qMakePair(targetEvtId, EventRelation::Annotation());
        if (relations.contains(lookupKey)) {
            relations[lookupKey].removeOne(reaction);
            emit q->updatedEvent(targetEvtId);
        }
    }
    q->onRedaction(*oldEvent, *ti);
    emit q->replacedEvent(ti.event(), rawPtr(oldEvent));
    return true;
}

/** Make a replaced event
 *
 * Takes \p target and returns a copy of it with content taken from
 * \p replacement. Disposal of the original event after that is on the caller.
 */
RoomEventPtr makeReplaced(const RoomEvent& target,
                          const RoomMessageEvent& replacement)
{
    const auto& targetReply = target.contentPart<QJsonObject>("m.relates_to");
    auto newContent = replacement.contentPart<QJsonObject>("m.new_content"_ls);
    if (!targetReply.empty()) {
        newContent["m.relates_to"] = targetReply;
    }
    auto originalJson = target.fullJson();
    originalJson[ContentKeyL] = newContent;

    auto unsignedData = originalJson.take(UnsignedKeyL).toObject();
    auto relations = unsignedData.take("m.relations"_ls).toObject();
    relations["m.replace"_ls] = replacement.id();
    unsignedData.insert(QStringLiteral("m.relations"), relations);
    originalJson.insert(UnsignedKey, unsignedData);

    return loadEvent<RoomEvent>(originalJson);
}

bool Room::Private::processReplacement(const RoomMessageEvent& newEvent)
{
    // Can't use findInTimeline because it returns a const iterator, and
    // we need to change the underlying TimelineItem.
    const auto pIdx = eventsIndex.constFind(newEvent.replacedEvent());
    if (pIdx == eventsIndex.cend())
        return false;

    Q_ASSERT(q->isValidIndex(*pIdx));

    auto& ti = timeline[Timeline::size_type(*pIdx - q->minTimelineIndex())];
    if (ti->replacedBy() == newEvent.id()) {
        qCDebug(STATE) << "Event" << ti->id() << "is already replaced with"
                       << newEvent.id();
        return true;
    }

    // Make a new event from the redacted JSON and put it in the timeline
    // instead of the redacted one. oldEvent will be deleted on return.
    auto oldEvent = ti.replaceEvent(makeReplaced(*ti, newEvent));
    qCDebug(STATE) << "Replaced" << oldEvent->id() << "with" << newEvent.id();
    emit q->replacedEvent(ti.event(), rawPtr(oldEvent));
    return true;
}

Connection* Room::connection() const
{
    Q_ASSERT(d->connection);
    return d->connection;
}

User* Room::localUser() const { return connection()->user(); }

/// Whether the event is a redaction or a replacement
inline bool isEditing(const RoomEventPtr& ep)
{
    Q_ASSERT(ep);
    if (is<RedactionEvent>(*ep))
        return true;
    if (auto* msgEvent = eventCast<RoomMessageEvent>(ep))
        return !msgEvent->replacedEvent().isEmpty();

    return false;
}

Room::Changes Room::Private::addNewMessageEvents(RoomEvents&& events)
{
    dropDuplicateEvents(events);
    if (events.empty())
        return Change::None;

    QElapsedTimer et;
    et.start();
    {
        // Pre-process redactions and edits so that events that get
        // redacted/replaced in the same batch landed in the timeline already
        // treated.
        // NB: We have to store redacting/replacing events to the timeline too -
        // see #220.
        auto it = std::find_if(events.begin(), events.end(), isEditing);
        for (const auto& eptr : RoomEventsRange(it, events.end())) {
            if (auto* r = eventCast<RedactionEvent>(eptr)) {
                // Try to find the target in the timeline, then in the batch.
                if (processRedaction(*r))
                    continue;
                if (auto targetIt = std::find_if(events.begin(), events.end(),
                        [id = r->redactedEvent()](const RoomEventPtr& ep) {
                            return ep->id() == id;
                        }); targetIt != events.end())
                    *targetIt = makeRedacted(**targetIt, *r);
                else
                    qCDebug(STATE)
                        << "Redaction" << r->id() << "ignored: target event"
                        << r->redactedEvent() << "is not found";
                // If the target event comes later, it comes already redacted.
            }
            if (auto* msg = eventCast<RoomMessageEvent>(eptr);
                    msg && !msg->replacedEvent().isEmpty()) {
                if (processReplacement(*msg))
                    continue;
                auto targetIt = std::find_if(events.begin(), it,
                        [id = msg->replacedEvent()](const RoomEventPtr& ep) {
                            return ep->id() == id;
                        });
                if (targetIt != it)
                    *targetIt = makeReplaced(**targetIt, *msg);
                else // FIXME: hide the replacing event when target arrives later
                    qCDebug(EVENTS)
                        << "Replacing event" << msg->id()
                        << "ignored: target event" << msg->replacedEvent()
                        << "is not found";
                // Same as with redactions above, the replaced event coming
                // later will come already with the new content.
            }
        }
    }

    // State changes arrive as a part of timeline; the current room state gets
    // updated before merging events to the timeline because that's what
    // clients historically expect. This may eventually change though if we
    // postulate that the current state is only current between syncs but not
    // within a sync.
    Changes roomChanges {};
    for (const auto& eptr : events)
        roomChanges |= q->processStateEvent(*eptr);

    auto timelineSize = timeline.size();
    size_t totalInserted = 0;
    for (auto it = events.begin(); it != events.end();) {
        auto nextPendingPair =
                    findFirstOf(it, events.end(), unsyncedEvents.begin(),
                                unsyncedEvents.end(), isEchoEvent);
                const auto& remoteEcho = nextPendingPair.first;
                const auto& localEcho = nextPendingPair.second;

        if (it != remoteEcho) {
            RoomEventsRange eventsSpan { it, remoteEcho };
            emit q->aboutToAddNewMessages(eventsSpan);
            auto insertedSize = moveEventsToTimeline(eventsSpan, Newer);
            totalInserted += insertedSize;
            auto firstInserted = syncEdge() - insertedSize;
            q->onAddNewTimelineEvents(firstInserted);
            emit q->addedMessages(firstInserted->index(),
                                  timeline.back().index());
        }
        if (remoteEcho == events.end())
            break;

        it = remoteEcho + 1;
        auto* nextPendingEvt = remoteEcho->get();
        const auto pendingEvtIdx = int(localEcho - unsyncedEvents.begin());
        if (localEcho->deliveryStatus() != EventStatus::ReachedServer) {
            localEcho->setReachedServer(nextPendingEvt->id());
            emit q->pendingEventChanged(pendingEvtIdx);
        }
        emit q->pendingEventAboutToMerge(nextPendingEvt, pendingEvtIdx);
        qCDebug(MESSAGES) << "Merging pending event from transaction"
                         << nextPendingEvt->transactionId() << "into"
                         << nextPendingEvt->id();
        auto transfer = fileTransfers.take(nextPendingEvt->transactionId());
        if (transfer.status != FileTransferInfo::None)
            fileTransfers.insert(nextPendingEvt->id(), transfer);
        // After emitting pendingEventAboutToMerge() above we cannot rely
        // on the previously obtained localEcho staying valid
        // because a signal handler may send another message, thereby altering
        // unsyncedEvents (see #286). Fortunately, unsyncedEvents only grows at
        // its back so we can rely on the index staying valid at least.
        unsyncedEvents.erase(unsyncedEvents.begin() + pendingEvtIdx);
        if (auto insertedSize = moveEventsToTimeline({ remoteEcho, it }, Newer)) {
            totalInserted += insertedSize;
            q->onAddNewTimelineEvents(syncEdge() - insertedSize);
        }
        emit q->pendingEventMerged();
    }
    // Events merged and transferred from `events` to `timeline` now.
    const auto from = syncEdge() - totalInserted;

    if (q->supportsCalls())
        for (auto it = from; it != syncEdge(); ++it)
            if (const auto* evt = it->viewAs<CallEventBase>())
                emit q->callEvent(q, evt);

    if (totalInserted > 0) {
        for (auto it = from; it != syncEdge(); ++it) {
            if (const auto* reaction = it->viewAs<ReactionEvent>()) {
                const auto& relation = reaction->relation();
                relations[{ relation.eventId, relation.type }] << reaction;
                emit q->updatedEvent(relation.eventId);
            }
        }

        qCDebug(MESSAGES) << "Room" << q->objectName() << "received"
                       << totalInserted << "new events; the last event is now"
                       << timeline.back();

        roomChanges |= updateStats(timeline.crbegin(), rev_iter_t(from));

        // If the local user's message(s) is/are first in the batch
        // and the fully read marker was right before it, promote
        // the fully read marker to the same event as the read receipt.
        const auto& firstWriterId = (*from)->senderId();
        if (firstWriterId == connection->userId()
            && q->fullyReadMarker().base() == from)
            roomChanges |=
                setFullyReadMarker(q->lastReadReceipt(firstWriterId).eventId);
    }

    Q_ASSERT(timeline.size() == timelineSize + totalInserted);
    if (totalInserted > 9 || et.nsecsElapsed() >= profilerMinNsecs())
        qCDebug(PROFILER) << "Added" << totalInserted << "new event(s) to"
                          << q->objectName() << "in" << et;
    return roomChanges;
}

void Room::Private::addHistoricalMessageEvents(RoomEvents&& events)
{
    QElapsedTimer et;
    et.start();
    const auto timelineSize = timeline.size();

    dropDuplicateEvents(events);
    if (events.empty())
        return;

    Changes changes {};
    // In case of lazy-loading new members may be loaded with historical
    // messages. Also, the cache doesn't store events with empty content;
    // so when such events show up in the timeline they should be properly
    // incorporated.
    for (const auto& eptr : events) {
        const auto& e = *eptr;
        if (e.isStateEvent()
            && !currentState.contains({ e.matrixType(), e.stateKey() })) {
            changes |= q->processStateEvent(e);
        }
    }

    emit q->aboutToAddHistoricalMessages(events);
    const auto insertedSize = moveEventsToTimeline(events, Older);
    const auto from = historyEdge() - insertedSize;

    qCDebug(STATE) << "Room" << displayname << "received" << insertedSize
                   << "past events; the oldest event is now" << timeline.front();
    q->onAddHistoricalTimelineEvents(from);
    emit q->addedMessages(timeline.front().index(), from->index());

    for (auto it = from; it != historyEdge(); ++it) {
        if (const auto* reaction = it->viewAs<ReactionEvent>()) {
            const auto& relation = reaction->relation();
            relations[{ relation.eventId, relation.type }] << reaction;
            emit q->updatedEvent(relation.eventId);
        }
    }
    Q_ASSERT(timeline.size() == timelineSize + insertedSize);
    if (insertedSize > 9 || et.nsecsElapsed() >= profilerMinNsecs())
        qCDebug(PROFILER) << "Added" << insertedSize << "historical event(s) to"
                          << q->objectName() << "in" << et;

    changes |= updateStats(from, historyEdge());
    if (changes)
        postprocessChanges(changes);
}

Room::Changes Room::processStateEvent(const RoomEvent& e)
{
    if (!e.isStateEvent())
        return Change::None;

    // Find a value (create an empty one if necessary) and get a reference
    // to it. Can't use getCurrentState<>() because it (creates and) returns
    // a stub if a value is not found, and what's needed here is a "real" event
    // or nullptr.
    auto& curStateEvent = d->currentState[{ e.matrixType(), e.stateKey() }];
    // Prepare for the state change
    // clang-format off
    const bool proceed = visit(e
        , [this, curStateEvent](const RoomMemberEvent& rme) {
            // clang-format on
            auto* oldRme = static_cast<const RoomMemberEvent*>(curStateEvent);
            auto* u = user(rme.userId());
            if (!u) { // Some terribly malformed user id?
                qCCritical(MAIN) << "Could not get a user object for"
                                 << rme.userId();
                return false; // Stay low and hope for the best...
            }
            const auto prevMembership = oldRme ? oldRme->membership()
                                               : Membership::Leave;
            switch (prevMembership) {
            case Membership::Invite:
                if (rme.membership() != prevMembership) {
                    d->usersInvited.removeOne(u);
                    Q_ASSERT(!d->usersInvited.contains(u));
                }
                break;
            case Membership::Join:
                if (rme.membership() == Membership::Join) {
                    // rename/avatar change or no-op
                    if (rme.newDisplayName()) {
                        emit memberAboutToRename(u, *rme.newDisplayName());
                        d->removeMemberFromMap(u);
                    }
                    if (!rme.newDisplayName() && !rme.newAvatarUrl()) {
                        qCWarning(MEMBERS)
                            << "No-op membership event for" << rme.userId()
                            << "- retaining the state";
                        qCWarning(MEMBERS) << "The event dump:" << rme;
                        return false;
                    }
                } else {
                    if (rme.membership() == Membership::Invite)
                        qCWarning(MAIN)
                            << "Membership change from Join to Invite:" << rme;
                    // whatever the new membership, it's no more Join
                    d->removeMemberFromMap(u);
                    emit userRemoved(u);
                }
                break;
            case Membership::Ban:
            case Membership::Knock:
            case Membership::Leave:
                if (rme.membership() == Membership::Invite
                    || rme.membership() == Membership::Join) {
                    d->membersLeft.removeOne(u);
                    Q_ASSERT(!d->membersLeft.contains(u));
                }
                break;
            case Membership::Undefined:
                ; // A warning will be dropped in the post-processing block below
            }
            return true;
            // clang-format off
        }
        , [this, curStateEvent]( const EncryptionEvent& ee) {
            // clang-format on
            auto* oldEncEvt =
                    static_cast<const EncryptionEvent*>(curStateEvent);
            if (ee.algorithm().isEmpty()) {
                qWarning(STATE)
                    << "The encryption event for room" << objectName()
                    << "doesn't have 'algorithm' specified - ignoring";
                return false;
            }
            if (oldEncEvt
                && oldEncEvt->encryption() != EncryptionEventContent::Undefined) {
                qCWarning(STATE) << "The room is already encrypted but a new"
                                    " room encryption event arrived - ignoring";
                return false;
            }
            return true;
            // clang-format off
        }
        , true); // By default, go forward with the state change
    // clang-format on
    if (!proceed) {
        if (!curStateEvent) // Remove the empty placeholder if one was created
            d->currentState.remove({ e.matrixType(), e.stateKey() });
        return Change::None;
    }

    // Change the state
    const auto* const oldStateEvent =
        std::exchange(curStateEvent, static_cast<const StateEventBase*>(&e));
    Q_ASSERT(!oldStateEvent
             || (oldStateEvent->matrixType() == e.matrixType()
                 && oldStateEvent->stateKey() == e.stateKey()));
    if (is<RoomMemberEvent>(e))
        qCDebug(MEMBERS) << "Updated room member state:" << e;
    else
        qCDebug(STATE) << "Updated room state:" << e;

    // Update internal structures as per the change and work out the return value

    // clang-format off
    const auto result = visit(e
        , [] (const RoomNameEvent&) {
            return Change::Name;
        }
        , [this, oldStateEvent] (const RoomCanonicalAliasEvent& cae) {
            // clang-format on
            setObjectName(cae.alias().isEmpty() ? d->id : cae.alias());
            const auto* oldCae =
                    static_cast<const RoomCanonicalAliasEvent*>(oldStateEvent);
            QStringList previousAltAliases {};
            if (oldCae) {
                previousAltAliases = oldCae->altAliases();
                if (!oldCae->alias().isEmpty())
                    previousAltAliases.push_back(oldCae->alias());
            }

            auto newAliases = cae.altAliases();
            if (!cae.alias().isEmpty())
                newAliases.push_front(cae.alias());

            connection()->updateRoomAliases(id(), previousAltAliases, newAliases);
            return Change::Aliases;
            // clang-format off
        }
        , [] (const RoomTopicEvent&) {
            return Change::Topic;
        }
        , [this] (const RoomAvatarEvent& evt) {
            if (d->avatar.updateUrl(evt.url()))
                emit avatarChanged();
            return Change::Avatar;
        }
        , [this,oldStateEvent] (const RoomMemberEvent& evt) {
            // clang-format on
            auto* u = user(evt.userId());
            const auto* oldMemberEvent =
                static_cast<const RoomMemberEvent*>(oldStateEvent);
            const auto prevMembership = oldMemberEvent
                                            ? oldMemberEvent->membership()
                                            : Membership::Leave;
            switch (evt.membership()) {
            case Membership::Join:
                if (prevMembership != Membership::Join) {
                    d->insertMemberIntoMap(u);
                    emit userAdded(u);
                } else {
                    if (evt.newDisplayName()) {
                        d->insertMemberIntoMap(u);
                        emit memberRenamed(u);
                    }
                    if (evt.newAvatarUrl())
                        emit memberAvatarChanged(u);
                }
                break;
            case Membership::Invite:
                if (!d->usersInvited.contains(u))
                    d->usersInvited.push_back(u);
                if (u == localUser() && evt.isDirect())
                    connection()->addToDirectChats(this, user(evt.senderId()));
                break;
            case Membership::Knock:
            case Membership::Ban:
            case Membership::Leave:
                if (!d->membersLeft.contains(u))
                    d->membersLeft.append(u);
                break;
            case Membership::Undefined:
                qCWarning(MEMBERS) << "Ignored undefined membership type";
            }
            return Change::Members;
            // clang-format off
        }
        , [this] (const EncryptionEvent&) {
            // As encryption can only be switched on once, emit the signal here
            // instead of aggregating and emitting in updateData()
            emit encryption();
            return Change::Other;
        }
        , [this] (const RoomTombstoneEvent& evt) {
            const auto successorId = evt.successorRoomId();
            if (auto* successor = connection()->room(successorId))
                emit upgraded(evt.serverMessage(), successor);
            else
                connectUntil(connection(), &Connection::loadedRoomState, this,
                    [this,successorId,serverMsg=evt.serverMessage()]
                    (Room* newRoom) {
                        if (newRoom->id() != successorId)
                            return false;
                        emit upgraded(serverMsg, newRoom);
                        return true;
                    });

            return Change::Other;
            // clang-format off
        }
        , Change::Other);
    // clang-format on
    Q_ASSERT(result != Change::None);
    return result;
}

Room::Changes Room::processEphemeralEvent(EventPtr&& event)
{
    Changes changes {};
    QElapsedTimer et;
    et.start();
    if (auto* evt = eventCast<TypingEvent>(event)) {
        d->usersTyping.clear();
        d->usersTyping.reserve(evt->users().size()); // Assume all are members
        for (const auto& userId : evt->users())
            if (isMember(userId))
                d->usersTyping.append(user(userId));

        if (evt->users().size() > 3 || et.nsecsElapsed() >= profilerMinNsecs())
            qCDebug(PROFILER)
                << "Processing typing events from" << evt->users().size()
                << "user(s) in" << objectName() << "took" << et;
        emit typingChanged();
    }
    if (auto* evt = eventCast<ReceiptEvent>(event)) {
        int totalReceipts = 0;
        const auto& eventsWithReceipts = evt->eventsWithReceipts();
        for (const auto& p : eventsWithReceipts) {
            totalReceipts += p.receipts.size();
            const auto newMarker = findInTimeline(p.evtId);
            if (newMarker == historyEdge())
                qCDebug(EPHEMERAL)
                    << "Event" << p.evtId
                    << "is not found; saving read receipt(s) anyway";
            // If the event is not found (most likely, because it's too old and
            // hasn't been fetched from the server yet) but there is a previous
            // marker for a user, keep the previous marker because read receipts
            // are not supposed to move backwards. Otherwise, blindly store
            // the event id for this user and update the read marker when/if
            // the event is fetched later on.
            const auto updatedCount = std::count_if(
                p.receipts.cbegin(), p.receipts.cend(),
                [this, &changes, &newMarker, &evtId = p.evtId](const auto& r) {
                    const auto change =
                        d->setLastReadReceipt(r.userId, newMarker,
                                              { evtId, r.timestamp });
                    changes |= change;
                    return change & Change::Any;
                });

            if (p.receipts.size() > 1)
                qCDebug(EPHEMERAL) << p.evtId << "marked as read for"
                                   << updatedCount << "user(s)";
            if (updatedCount < p.receipts.size())
                qCDebug(EPHEMERAL) << p.receipts.size() - updatedCount
                                   << "receipts were skipped";
        }
        if (eventsWithReceipts.size() > 3 || totalReceipts > 10
            || et.nsecsElapsed() >= profilerMinNsecs())
            qCDebug(PROFILER) << "Processing" << totalReceipts
                              << "receipt(s) on" << eventsWithReceipts.size()
                              << "event(s) in" << objectName() << "took" << et;
    }
    return changes;
}

Room::Changes Room::processAccountDataEvent(EventPtr&& event)
{
    Changes changes {};
    if (auto* evt = eventCast<TagEvent>(event)) {
        d->setTags(evt->tags());
        changes |= Change::Tags;
    }

    if (auto* evt = eventCast<const ReadMarkerEvent>(event))
        changes |= d->setFullyReadMarker(evt->event_id());

    // For all account data events
    auto& currentData = d->accountData[event->matrixType()];
    // A polymorphic event-specific comparison might be a bit more
    // efficient; maaybe do it another day
    if (!currentData || currentData->contentJson() != event->contentJson()) {
        emit accountDataAboutToChange(event->matrixType());
        currentData = move(event);
        qCDebug(STATE) << "Updated account data of type"
                       << currentData->matrixType();
        emit accountDataChanged(currentData->matrixType());
        // TODO: Drop AccountDataChange in 0.8
        // NB: GCC (at least 10) only accepts QT_IGNORE_DEPRECATIONS around
        // a statement, not within a statement
        QT_IGNORE_DEPRECATIONS(changes |= Change::AccountData | Change::Other;)
    }
    return changes;
}

template <typename ContT>
Room::Private::users_shortlist_t
Room::Private::buildShortlist(const ContT& users) const
{
    // To calculate room display name the spec requires to sort users
    // lexicographically by state_key (user id) and use disambiguated
    // display names of two topmost users excluding the current one to render
    // the name of the room. The below code selects 3 topmost users,
    // slightly extending the spec.
    users_shortlist_t shortlist {}; // Prefill with nullptrs
    std::partial_sort_copy(
        users.begin(), users.end(), shortlist.begin(), shortlist.end(),
        [this](const User* u1, const User* u2) {
            // localUser(), if it's in the list, is sorted
            // below all others
            return isLocalUser(u2) || (!isLocalUser(u1) && u1->id() < u2->id());
        });
    return shortlist;
}

Room::Private::users_shortlist_t
Room::Private::buildShortlist(const QStringList& userIds) const
{
    QList<User*> users;
    users.reserve(userIds.size());
    for (const auto& h : userIds)
        users.push_back(q->user(h));
    return buildShortlist(users);
}

QString Room::Private::calculateDisplayname() const
{
    // CS spec, section 13.2.2.5 Calculating the display name for a room
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

    // 3. m.room.aliases - only local aliases, subject for further removal
    const auto aliases = q->aliases();
    if (!aliases.isEmpty())
        return aliases.front();

    // 4. m.heroes and m.room.member
    // From here on, we use a more general algorithm than the spec describes
    // in order to provide back-compatibility with pre-MSC688 servers.

    // Supplementary code: build the shortlist of users whose names
    // will be used to construct the room name. Takes into account MSC688's
    // "heroes" if available.
    const bool localUserIsIn = joinState == JoinState::Join;
    const bool emptyRoom =
        membersMap.isEmpty()
        || (membersMap.size() == 1 && isLocalUser(*membersMap.cbegin()));
    const bool nonEmptySummary = summary.heroes && !summary.heroes->empty();
    auto shortlist = nonEmptySummary ? buildShortlist(*summary.heroes)
                                     : !emptyRoom ? buildShortlist(membersMap)
                                                  : users_shortlist_t {};

    // When the heroes list is there, we can rely on it. If the heroes list is
    // missing, the below code gathers invited, or, if there are no invitees,
    // left members.
    if (!shortlist.front() && localUserIsIn)
        shortlist = buildShortlist(usersInvited);

    if (!shortlist.front())
        shortlist = buildShortlist(membersLeft);

    QStringList names;
    for (auto u : shortlist) {
        if (u == nullptr || isLocalUser(u))
            break;
        // Only disambiguate if the room is not empty
        names.push_back(u->displayname(emptyRoom ? nullptr : q));
    }

    const auto usersCountExceptLocal =
        !emptyRoom
            ? q->joinedCount() - int(joinState == JoinState::Join)
            : !usersInvited.empty()
                  ? usersInvited.count()
                  : membersLeft.size() - int(joinState == JoinState::Leave);
    if (usersCountExceptLocal > int(shortlist.size()))
        names << tr(
            "%Ln other(s)",
            "Used to make a room name from user names: A, B and _N others_",
            usersCountExceptLocal - int(shortlist.size()));
    const auto namesList = QLocale().createSeparatedList(names);

    // Room members
    if (!emptyRoom)
        return namesList;

    // (Spec extension) Invited users
    if (!usersInvited.empty())
        return tr("Empty room (invited: %1)").arg(namesList);

    // Users that previously left the room
    if (!membersLeft.isEmpty())
        return tr("Empty room (was: %1)").arg(namesList);

    // Fail miserably
    return tr("Empty room (%1)").arg(id);
}

void Room::Private::updateDisplayname()
{
    auto swappedName = calculateDisplayname();
    if (swappedName != displayname) {
        emit q->displaynameAboutToChange(q);
        swap(displayname, swappedName);
        qCDebug(MAIN) << q->objectName() << "has changed display name from"
                     << swappedName << "to" << displayname;
        emit q->displaynameChanged(q, swappedName);
    }
}

QJsonObject Room::Private::toJson() const
{
    QElapsedTimer et;
    et.start();
    QJsonObject result;
    addParam<IfNotEmpty>(result, QStringLiteral("summary"), summary);
    {
        QJsonArray stateEvents;

        for (const auto* evt : currentState) {
            Q_ASSERT(evt->isStateEvent());
            if ((evt->isRedacted() && !is<RoomMemberEvent>(*evt))
                || evt->contentJson().isEmpty())
                continue;

            auto json = evt->fullJson();
            auto unsignedJson = evt->unsignedJson();
            unsignedJson.remove(QStringLiteral("prev_content"));
            json[UnsignedKeyL] = unsignedJson;
            stateEvents.append(json);
        }

        const auto stateObjName = joinState == JoinState::Invite
                                      ? QStringLiteral("invite_state")
                                      : QStringLiteral("state");
        result.insert(stateObjName,
                      QJsonObject { { QStringLiteral("events"), stateEvents } });
    }

    if (!accountData.empty()) {
        QJsonArray accountDataEvents;
        for (const auto& e : accountData) {
            if (!e.second->contentJson().isEmpty())
                accountDataEvents.append(e.second->fullJson());
        }
        result.insert(QStringLiteral("account_data"),
                      QJsonObject {
                          { QStringLiteral("events"), accountDataEvents } });
    }

    if (const auto& readReceipt = q->lastReadReceipt(connection->userId());
        !readReceipt.eventId.isEmpty()) //
    {
        result.insert(
            QStringLiteral("ephemeral"),
            QJsonObject {
                { QStringLiteral("events"),
                  QJsonArray { ReceiptEvent({ { readReceipt.eventId,
                                                { { connection->userId(),
                                                    readReceipt.timestamp } } } })
                                   .fullJson() } } });
    }

    result.insert(UnreadNotificationsKey,
                  QJsonObject { { PartiallyReadCountKey,
                                  countFromStats(partiallyReadStats) },
                                { HighlightCountKey, serverHighlightCount } });
    result.insert(NewUnreadCountKey, countFromStats(unreadStats));

    if (et.elapsed() > 30)
        qCDebug(PROFILER) << "Room::toJson() for" << q->objectName() << "took"
                          << et;

    return result;
}

QJsonObject Room::toJson() const { return d->toJson(); }

MemberSorter Room::memberSorter() const { return MemberSorter(this); }

bool MemberSorter::operator()(User* u1, User* u2) const
{
    return operator()(u1, room->disambiguatedMemberName(u2->id()));
}

bool MemberSorter::operator()(User* u1, QStringView u2name) const
{
    auto n1 = room->disambiguatedMemberName(u1->id());
    if (n1.startsWith('@'))
        n1.remove(0, 1);
    const auto n2 = u2name.mid(u2name.startsWith('@') ? 1 : 0)
#if QT_VERSION_MAJOR < 6
        .toString() // Qt 5 doesn't have QStringView::localeAwareCompare
#endif
        ;

    return n1.localeAwareCompare(n2) < 0;
}

void Room::activateEncryption()
{
    if(usesEncryption()) {
        qCWarning(E2EE) << "Room" << objectName() << "is already encrypted";
        return;
    }
    setState<EncryptionEvent>(EncryptionEventContent::MegolmV1AesSha2);
}

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
#include "database.h"
#include "eventstats.h"
#include "keyverificationsession.h"
#include "logging_categories_p.h"
#include "qt_connection_util.h"
#include "quotient_common.h"
#include "ranges_extras.h"
#include "roommember.h"
#include "roomstateview.h"
#include "syncdata.h"
#include "user.h"

#include "csapi/account-data.h"
#include "csapi/banning.h"
#include "csapi/inviting.h"
#include "csapi/kicking.h"
#include "csapi/leaving.h"
#include "csapi/read_markers.h"
#include "csapi/receipts.h"
#include "csapi/redaction.h"
#include "csapi/room_send.h"
#include "csapi/room_state.h"
#include "csapi/room_upgrades.h"
#include "csapi/rooms.h"
#include "csapi/tags.h"

#include "e2ee/e2ee_common.h"
#include "e2ee/qolmaccount.h"
#include "e2ee/qolminboundsession.h"

#include "events/callevents.h"
#include "events/encryptionevent.h"
#include "events/event.h"
#include "events/reactionevent.h"
#include "events/receiptevent.h"
#include "events/redactionevent.h"
#include "events/roomavatarevent.h"
#include "events/roomcanonicalaliasevent.h"
#include "events/roomcreateevent.h"
#include "events/roommemberevent.h"
#include "events/roompowerlevelsevent.h"
#include "events/roomtombstoneevent.h"
#include "events/simplestateevents.h"
#include "events/typingevent.h"
#include "jobs/downloadfilejob.h"
#include "jobs/mediathumbnailjob.h"

// NB: since Qt 6, moc_room.cpp needs User fully defined
#include "moc_room.cpp" // NOLINT(bugprone-suspicious-include)

#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringBuilder> // for efficient string concats (operator%)
#include <QtCore/QTemporaryFile>

#include <array>
#include <cmath>
#include <functional>

using namespace Quotient;
using namespace std::placeholders;
using std::llround;

enum EventsPlacement : int { Older = -1, Newer = 1 };

class Q_DECL_HIDDEN Room::Private {
public:
    Private(Connection* c, QString id_, JoinState initialJoinState)
        : connection(c)
        , id(std::move(id_))
        , joinState(initialJoinState)
    {}

    Room* q = nullptr;

    Connection* connection;
    QString id;
    JoinState joinState;
    RoomSummary summary = { {}, 0, {} };
    /// The state of the room at timeline position before-0
    std::unordered_map<StateEventKey, StateEventPtr> baseState;
    /// The state of the room at syncEdge()
    /// \sa syncEdge
    RoomStateView currentState;
    /// Servers with aliases for this room except the one of the local user
    /// \sa Room::remoteAliases
    QSet<QString> aliasServers;

    Timeline timeline;
    PendingEvents unsyncedEvents;
    QHash<QString, TimelineItem::index_t> eventsIndex;
    // A map from event id/relation type pairs to a vector of event pointers. Not using QMultiHash,
    // because we want to quickly return a number of relations for a given event without enumerating
    // them.
    QHash<std::pair<QString, QString>, RelatedEvents> relations;
    QString displayname;
    Avatar avatar;
    QHash<QString, Notification> notifications;
    qsizetype serverHighlightCount = 0;
    // Starting up with estimate event statistics as there's zero knowledge
    // about the timeline.
    EventStats partiallyReadStats {}, unreadStats {};

    // For storing a list of current member names for the purpose of disambiguation.
    QMultiHash<QString, QString> memberNameMap;
    QStringList membersInvited;
    QStringList membersLeft;
    QStringList membersTyping;

    QHash<QString, QSet<QString>> eventIdReadUsers;
    bool displayed = false;
    QString firstDisplayedEventId;
    QString lastDisplayedEventId;
    QHash<QString, ReadReceipt> lastReadReceipts;
    QString fullyReadUntilEventId;
    TagsMap tags;
    std::unordered_map<QString, EventPtr> accountData;
    //! \brief Previous (i.e. next towards the room beginning) batch token
    //!
    //! "Emptiness" of this can have two forms. If prevBatch.has_value() it means the library
    //! assumes the previous batch to exist on the server, even though it might not know the token
    //! (hence initialisation with a null string). If <tt>prevBatch == std::nullopt</tt> it means
    //! that the server previously reported that all events have been loaded and there's no point in
    //! requesting further historical batches.
    std::optional<QString> prevBatch = QString();
    int lastRequestedHistorySize = 0;
    JobHandle<GetRoomEventsJob> eventsHistoryJob;
    JobHandle<GetMembersByRoomJob> allMembersJob;
    //! Map from megolm sessionId to set of eventIds
    std::unordered_map<QString, QSet<QString>> undecryptedEvents;
    //! Map from event id of the request event to the session object
    QHash<QString, KeyVerificationSession *> keyVerificationSessions;
    QPointer<KeyVerificationSession> pendingKeyVerificationSession;
    // The timestamp of the last message loaded from the database;
    qlonglong lastTs = LONG_LONG_MAX;
    bool isLoadingMessages = false;

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

    Changes setSummary(RoomSummary&& newSummary);

    void preprocessStateEvent(const RoomEvent& newEvent,
                              const RoomEvent* curEvent);
    Change processStateEvent(const RoomEvent& curEvent,
                             const RoomEvent* oldEvent);

    void insertMemberIntoMap(const QString& memberId);
    void removeMemberFromMap(const QString& memberId);

    // This updates the room displayname field (which is the way a room
    // should be shown in the room list); called whenever the list of
    // members, the room name (m.room.name) or canonical alias change.
    void updateDisplayname();
    // This is used by updateDisplayname() but only calculates the new name
    // without any updates.
    QString calculateDisplayname() const;

    rev_iter_t historyEdge() const { return timeline.crend(); }
    Timeline::const_iterator syncEdge() const { return timeline.cend(); }

    JobHandle<GetRoomEventsJob> getPreviousContent(int limit = 10, const QString &filter = {});

    Changes updateStateFrom(StateEvents&& events)
    {
        Changes changes {};
        if (!events.empty()) {
            QElapsedTimer et;
            et.start();
            for (auto&& eptr : std::move(events)) {
                const auto& evt = *eptr;
                Q_ASSERT(evt.isStateEvent());
                if (auto change = q->processStateEvent(evt); change) {
                    changes |= change;
                    baseState[{ evt.matrixType(), evt.stateKey() }] =
                        std::move(eptr);
                }
            }
            if (events.size() > 9 || et.nsecsElapsed() >= ProfilerMinNsecs)
                qCDebug(PROFILER)
                    << "Updated" << q->objectName() << "room state from"
                    << events.size() << "event(s) in" << et;
        }
        return changes;
    }
    void addRelation(const ReactionEvent& reactionEvt);
    void addRelations(auto from, auto to)
    {
        for (auto it = from; it != to; ++it)
            if (const auto* reaction = it->template viewAs<ReactionEvent>())
                addRelation(*reaction);
    }

    Changes addNewMessageEvents(RoomEvents&& events);
    std::pair<Changes, rev_iter_t> addHistoricalMessageEvents(RoomEvents&& events, bool fromDb = false);
    void storeInDb(const RoomEvents &events);
    int loadFromDb(int limit);

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
    void dropExtraneousEvents(RoomEvents& events) const;
    void decryptIncomingEvents(RoomEvents& events);

    //! \brief update last receipt record for a given user
    //!
    //! \return previous event id of the receipt if the new receipt changed
    //!         it, or `std::nullopt` if no change took place
    std::optional<QString> setLastReadReceipt(const QString& userId, rev_iter_t newMarker,
                                              ReadReceipt newReceipt = {});
    Changes setLocalLastReadReceipt(const rev_iter_t& newMarker,
                                    ReadReceipt newReceipt = {},
                                    bool deferStatsUpdate = false);
    Changes setFullyReadMarker(const QString &eventId);
    Changes updateStats(const rev_iter_t& from, const rev_iter_t& to);
    bool markMessagesAsRead(const rev_iter_t& upToMarker);

    void getAllMembers();

    const PendingEventItem& sendEvent(RoomEventPtr&& event);

    QString doPostFile(RoomEventPtr &&msgEvent, const QUrl &localUrl);

    PendingEvents::iterator addAsPending(RoomEventPtr&& event);

    const PendingEventItem& doSendEvent(PendingEvents::iterator eventItemIter);

    void onEventReachedServer(PendingEvents::iterator eventItemIter, const QString& eventId);
    void onEventSendingFailure(PendingEvents::iterator eventItemIter, const BaseJob* call = nullptr);

    SetRoomStateWithKeyJob* requestSetState(const QString& evtType,
                                            const QString& stateKey,
                                            const QJsonObject& contentJson)
    {
        //            if (event.roomId().isEmpty())
        //                event.setRoomId(id);
        //            if (event.senderId().isEmpty())
        //                event.setSender(connection->userId());
        // TODO: Queue up state events sending (see #133).
        return connection->callApi<SetRoomStateWithKeyJob>(id, evtType, stateKey,
                                                           contentJson);
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

    bool isLocalMember(const QString& memberId) const { return memberId == connection->userId(); }

    std::unordered_map<QByteArray, QOlmInboundGroupSession> groupSessions;
    std::optional<QOlmOutboundGroupSession> currentOutboundMegolmSession = {};

    bool addInboundGroupSession(QByteArray sessionId, QByteArray sessionKey,
                                const QString& senderId,
                                const QByteArray& olmSessionId, const QByteArray& senderKey, const QByteArray& senderEdKey)
    {
        if (groupSessions.contains(sessionId)) {
            qCWarning(E2EE) << "Inbound Megolm session" << sessionId << "already exists";
            return false;
        }

        auto expectedMegolmSession = QOlmInboundGroupSession::create(sessionKey);
        Q_ASSERT(expectedMegolmSession.has_value());
        auto&& megolmSession = *expectedMegolmSession;
        if (megolmSession.sessionId() != sessionId) {
            qCWarning(E2EE) << "Session ID mismatch in m.room_key event";
            return false;
        }
        megolmSession.setSenderId(senderId);
        megolmSession.setOlmSessionId(olmSessionId);
        qCWarning(E2EE) << "Adding inbound session" << sessionId;
        connection->saveMegolmSession(q, megolmSession, senderKey, senderEdKey);
        groupSessions.try_emplace(sessionId, std::move(megolmSession));
        return true;
    }

    QString groupSessionDecryptMessage(const QByteArray& ciphertext,
                                       const QByteArray& sessionId,
                                       const QString& eventId,
                                       const QDateTime& timestamp,
                                       const QString& senderId)
    {
        auto groupSessionIt = groupSessions.find(sessionId);
        if (groupSessionIt == groupSessions.end()) {
            // qCWarning(E2EE) << "Unable to decrypt event" << eventId
            //               << "The sender's device has not sent us the keys for "
            //                  "this message";
            // TODO: request the keys
            return {};
        }
        auto& senderSession = groupSessionIt->second;
        if (senderSession.senderId() != "BACKUP"_ls && senderSession.senderId() != senderId) {
            qCWarning(E2EE) << "Sender from event does not match sender from session";
            return {};
        }
        auto decryptResult = senderSession.decrypt(ciphertext);
        if(!decryptResult) {
            qCWarning(E2EE) << "Unable to decrypt event" << eventId
            << "with matching megolm session:" << decryptResult.error();
            return {};
        }
        const auto& [content, index] = *decryptResult;
        const auto& [recordEventId, ts] =
            q->connection()->database()->groupSessionIndexRecord(
                q->id(), QString::fromLatin1(senderSession.sessionId()), index);
        if (recordEventId.isEmpty()) {
            q->connection()->database()->addGroupSessionIndexRecord(
                q->id(), QString::fromLatin1(senderSession.sessionId()), index, eventId,
                timestamp.toMSecsSinceEpoch());
        } else {
            if ((eventId != recordEventId)
                || (ts != timestamp.toMSecsSinceEpoch())) {
                qCWarning(E2EE) << "Detected a replay attack on event" << eventId;
                return {};
            }
        }
        return QString::fromUtf8(content);
    }

    bool shouldRotateMegolmSession() const
    {
        const auto* encryptionConfig = currentState.get<EncryptionEvent>();
        if (!encryptionConfig || !encryptionConfig->useEncryption())
            return false;

        const auto rotationInterval = encryptionConfig->rotationPeriodMs();
        const auto rotationMessageCount = encryptionConfig->rotationPeriodMsgs();
        return currentOutboundMegolmSession->messageCount()
                   >= rotationMessageCount
               || currentOutboundMegolmSession->creationTime().addMSecs(
                      rotationInterval)
                      < QDateTime::currentDateTime();
    }

    bool hasValidMegolmSession() const
    {
        return q->usesEncryption() && currentOutboundMegolmSession.has_value();
    }

    void createMegolmSession() {
        qCDebug(E2EE) << "Creating new outbound megolm session for room "
                      << q->objectName();
        currentOutboundMegolmSession.emplace();
        connection->database()->saveCurrentOutboundMegolmSession(
            id, *currentOutboundMegolmSession);

        addInboundGroupSession(currentOutboundMegolmSession->sessionId(),
                               currentOutboundMegolmSession->sessionKey(),
                               q->localMember().id(), QByteArrayLiteral("SELF"),
                               connection->curveKeyForUserDevice(connection->userId(), connection->deviceId()).toLatin1(),
                               connection->edKeyForUserDevice(connection->userId(), connection->deviceId()).toLatin1());
    }

    QMultiHash<QString, QString> getDevicesWithoutKey() const
    {
        QMultiHash<QString, QString> devices;
        for (const auto& user : memberNameMap.values() + membersInvited)
            for (const auto& deviceId : connection->devicesForUser(user))
                devices.insert(user, deviceId);

        return connection->database()->devicesWithoutKey(
            id, devices, currentOutboundMegolmSession->sessionId());
    }

private:
    Room::Timeline::size_type mergePendingEvent(PendingEvents::iterator localEchoIt,
                                                RoomEvents::iterator remoteEchoIt);

    using users_shortlist_t = std::array<QString, 3>;
    users_shortlist_t buildShortlist(const QStringList& userIds) const;
};

Room::Room(Connection* connection, QString id, JoinState initialJoinState)
    : QObject(connection), d(new Private(connection, id, initialJoinState))
{
    setObjectName(id);
    // See "Accessing the Public Class" section in
    // https://marcmutz.wordpress.com/translated-articles/pimp-my-pimpl-%E2%80%94-reloaded/
    d->q = this;
    d->displayname = d->calculateDisplayname(); // Set initial "Empty room" name
    if (connection->encryptionEnabled()) {
        connect(this, &Room::encryption, this,
                [this, connection] { connection->encryptionUpdate(this); });
        connect(this, &Room::memberListChanged, this, [this, connection] {
            if(usesEncryption()) {
                connection->encryptionUpdate(this, d->membersInvited);
            }
        });
        d->groupSessions = connection->loadRoomMegolmSessions(this);
        d->currentOutboundMegolmSession =
            connection->database()->loadCurrentOutboundMegolmSession(id);
        if (d->currentOutboundMegolmSession
            && d->shouldRotateMegolmSession()) {
            d->currentOutboundMegolmSession.reset();
        }
        connect(this, &Room::memberLeft, this, [this] {
            if (d->hasValidMegolmSession()) {
                qCDebug(E2EE)
                    << "Rotating the megolm session because a user left";
                d->createMegolmSession();
            }
        });

        connect(this, &Room::beforeDestruction, this, [id, connection] {
            connection->database()->clearRoomData(id);
        });
    }

    auto query = connection->database()->prepareQuery("SELECT token FROM event_batch_tokens WHERE roomId=:roomId;"_ls);
    query.bindValue(":roomId"_ls, id);
    connection->database()->execute(query);
    if (query.next()) {
        d->prevBatch = query.value("token"_ls).toString();
    };

    qCDebug(STATE) << "New" << terse << initialJoinState << "Room:" << id;
}

Room::~Room() { delete d; }

const QString& Room::id() const { return d->id; }

QString Room::version() const
{
    const auto v = currentState().query(&RoomCreateEvent::version);
    return v && !v->isEmpty() ? *v : QStringLiteral("1");
}

bool Room::isUnstable() const
{
    return connection()->capabilitiesReady()
           && !connection()->stableRoomVersions().contains(version());
}

QString Room::predecessorId() const
{
    if (const auto* evt = currentState().get<RoomCreateEvent>())
        return evt->predecessor().roomId;

    return {};
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
    return currentState().queryOr(&RoomTombstoneEvent::successorRoomId,
                                  QString());
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

int Room::requestedHistorySize() const
{
    return eventsHistoryJob() != nullptr ? d->lastRequestedHistorySize : 0;
}

bool Room::allHistoryLoaded() const
{
    if (timelineSize() == 0) {
        return false;
    }
    return messageEvents().front()->is<const RoomCreateEvent>();
}

QString Room::name() const
{
    return currentState().content<RoomNameEvent>().value;
}

QStringList Room::aliases() const
{
    if (const auto* evt = currentState().get<RoomCanonicalAliasEvent>()) {
        auto result = evt->altAliases();
        if (!evt->alias().isEmpty())
            result << evt->alias();
        return result;
    }
    return {};
}

QStringList Room::altAliases() const
{
    return currentState().content<RoomCanonicalAliasEvent>().altAliases;
}

QString Room::canonicalAlias() const
{
    return currentState().content<RoomCanonicalAliasEvent>().canonicalAlias;
}

QString Room::displayName() const { return d->displayname; }

QStringList Room::pinnedEventIds() const {
    return currentState().content<RoomPinnedEventsEvent>().value;
}

QVector<const Quotient::RoomEvent*> Quotient::Room::pinnedEvents() const
{
    QVector<const RoomEvent*> pinnedEvents;
    for (const auto eventIds = pinnedEventIds(); const auto& evtId : eventIds)
        if (const auto& it = findInTimeline(evtId); it != historyEdge())
            pinnedEvents.append(it->event());

    return pinnedEvents;
}

QString Room::displayNameForHtml() const
{
    return displayName().toHtmlEscaped();
}

void Room::refreshDisplayName() { d->updateDisplayname(); }

QString Room::topic() const
{
    return currentState().content<RoomTopicEvent>().value;
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
    for (const auto dcMembers = directChatMembers(); const auto& m : dcMembers)
        if (m != localMember())
            return memberAvatar(m.id()).get(connection(), width, height,
                                            [this] { emit avatarChanged(); });

    return {};
}

RoomMember Room::localMember() const { return member(connection()->userId()); }

RoomMember Room::member(const QString& userId) const
{
    if (userId.isEmpty()) {
        return {};
    }
    return RoomMember(this, currentState().get<RoomMemberEvent>(userId));
}

QList<RoomMember> Room::joinedMembers() const
{
    QList<RoomMember> joinedMembers;
    joinedMembers.reserve(joinedCount());

    const auto memberEvents = currentState().eventsOfType(RoomMemberEvent::TypeId);
    for (const auto event : memberEvents) {
        if (const auto memberEvent = eventCast<const RoomMemberEvent>(event);
            memberEvent->membership() == Membership::Join) {
            joinedMembers.append(RoomMember(this, memberEvent));
        }
    }
    return joinedMembers;
}

QList<RoomMember> Room::members() const {
    QList<RoomMember> members;
    members.reserve(totalMemberCount());

    const auto memberEvents = currentState().eventsOfType(RoomMemberEvent::TypeId);
    for (const auto event : memberEvents) {
        if (const auto memberEvent = eventCast<const RoomMemberEvent>(event)) {
            members.append(RoomMember(this, memberEvent));
        }
    }
    return members;
}

QList<RoomMember> Room::membersTyping() const
{
    QList<RoomMember> members;
    members.reserve(d->membersTyping.count());
    for (const auto &memberId : d->membersTyping) {
        members.append(member(memberId));
    }
    return members;
}

QList<RoomMember> Room::otherMembersTyping() const
{
    auto memberTyping = membersTyping();
    memberTyping.removeAll(localMember());
    return memberTyping;
}

QStringList Room::joinedMemberIds() const
{
    QStringList ids;
    ids.reserve(joinedCount());

    const auto memberEvents = currentState().eventsOfType(RoomMemberEvent::TypeId);
    for (const auto event : memberEvents) {
        if (const auto memberEvent = eventCast<const RoomMemberEvent>(event);
            memberEvent->membership() == Membership::Join) {
            ids.append(memberEvent->userId());
        }
    }
    return ids;
}

QStringList Room::memberIds() const
{
    QStringList ids;
    ids.reserve(totalMemberCount());

    const auto memberEvents = currentState().eventsOfType(RoomMemberEvent::TypeId);
    for (const auto event : memberEvents) {
        if (const auto memberEvent = eventCast<const RoomMemberEvent>(event)) {
            ids.append(memberEvent->userId());
        }
    }
    return ids;
}

bool Room::needsDisambiguation(const QString& userId) const
{
    return d->memberNameMap.count(member(userId).name()) > 1;
}

Membership Room::memberState(const QString& userId) const
{
    return currentState().queryOr(userId, &RoomMemberEvent::membership,
                                  Membership::Leave);
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

std::optional<QString> Room::Private::setLastReadReceipt(const QString& userId, rev_iter_t newMarker,
                                                         ReadReceipt newReceipt)
{
    if (newMarker == historyEdge() && !newReceipt.eventId.isEmpty())
        newMarker = q->findInTimeline(newReceipt.eventId);
    if (newMarker != historyEdge()) {
        // Try to auto-promote the read marker over the user's own messages
        // (switch to direct iterators for that).
        const auto eagerMarker =
            find_if(newMarker.base(), syncEdge(),
                    [userId](const TimelineItem& ti) { return ti->senderId() != userId; });
        // eagerMarker is now just after the desired event for newMarker
        if (eagerMarker != newMarker.base()) {
            newMarker = rev_iter_t(eagerMarker);
            qDebug(EPHEMERAL) << "Auto-promoted read receipt for" << userId
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
    // This logic tackles, in particular, the case when the new event is not
    // found (most likely, because it's too old and hasn't been fetched from
    // the server yet) but there is a previous marker for a user; in that case,
    // the previous marker is kept because read receipts are not supposed
    // to move backwards. If neither new nor old event is found, the new receipt
    // is blindly stored, in a hope it's also "newer" in the timeline.
    // NB: with reverse iterators, timeline history edge >= sync edge
    if (prevEventId == newReceipt.eventId
        || newMarker > q->findInTimeline(prevEventId))
        return {};

    // Finally make the change

    auto oldEventReadUsersIt =
        eventIdReadUsers.find(prevEventId); // clazy:exclude=detaching-member
    if (oldEventReadUsersIt != eventIdReadUsers.end()) {
        oldEventReadUsersIt->remove(userId);
        if (oldEventReadUsersIt->isEmpty())
            eventIdReadUsers.erase(oldEventReadUsersIt);
    }
    eventIdReadUsers[newReceipt.eventId].insert(userId);
    storedReceipt = std::move(newReceipt);

    {
        auto dbg = qDebug(EPHEMERAL); // NB: qCDebug can't be used like that
        dbg << "The new read receipt for" << userId << "is now at";
        if (newMarker == historyEdge())
            dbg << storedReceipt.eventId;
        else
            dbg << *newMarker;
    }

    // NB: This method, unlike setLocalLastReadReceipt, doesn't emit
    // lastReadEventChanged() to avoid numerous emissions when many read
    // receipts arrive. It can be called thousands of times during an initial
    // sync, e.g.
    return prevEventId;
}

Room::Changes Room::Private::setLocalLastReadReceipt(const rev_iter_t& newMarker,
                                                     ReadReceipt newReceipt,
                                                     bool deferStatsUpdate)
{
    auto prevEventId = setLastReadReceipt(connection->userId(), newMarker,
                                          std::move(newReceipt));
    if (!prevEventId)
        return Change::None;
    Changes changes = Change::Other;
    if (!deferStatsUpdate) {
        if (unreadStats.updateOnMarkerMove(q, q->findInTimeline(*prevEventId),
                                           newMarker)) {
            qDebug(MESSAGES)
                << "Updated unread event statistics in" << q->objectName()
                << "after moving the local read receipt:" << unreadStats;
            changes |= Change::UnreadStats;
        }
        Q_ASSERT(unreadStats.isValidFor(q, newMarker)); // post-check
    }
    emit q->lastReadEventChanged({ connection->userId() });
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
        && setLocalLastReadReceipt(fullyReadMarker, {}, true)) {
        changes |= Change::Other;
        readReceiptMarker = q->localReadReceiptMarker();
        qCInfo(MESSAGES) << "The local m.read receipt was behind m.fully_read "
                            "marker - it's now corrected to be at index"
                         << readReceiptMarker->index();
    }

    if (fullyReadMarker < from)
        return Change::None; // What's arrived is already fully read

    // If there's no read marker in the whole room, initialise it
    // REMOVEME: it's not the library's business; the room might be offscreen,
    // or the creation event not shown, whatever. Let the clients tackle that
    // properly.
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
        qCDebug(MESSAGES).nospace()
            << "Recalculated unread event statistics in " << q->objectName()
            << ": " << unreadStats;
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

    Changes changes = Change::Other;
    if (const auto rm = q->fullyReadMarker(); rm != historyEdge()) {
        // Pull read receipt if it's behind, and update statistics
        changes |= setLocalLastReadReceipt(rm);
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
    return changes;
}

void Room::setReadReceipt(const QString& atEventId)
{
    if (const auto changes =
            d->setLocalLastReadReceipt(historyEdge(), { atEventId })) {
        connection()->callApi<PostReceiptJob>(BackgroundRequest, id(),
                                              QStringLiteral("m.read"),
                                              QString::fromUtf8(QUrl::toPercentEncoding(atEventId)));
        d->postprocessChanges(changes);
    } else
        qCDebug(EPHEMERAL) << "The new read receipt for" << localMember().id()
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

    if (const auto* plEvt = currentState().get<RoomPowerLevelsEvent>()) {
        const auto currentUserLevel =
            plEvt->powerLevelForUser(localMember().id());
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
           && evt.senderId() != localMember().id();
}

Notification Room::notificationFor(const TimelineItem &ti) const
{
    return d->notifications.value(ti->id());
}

Notification Room::checkForNotifications(const TimelineItem &ti)
{
    return { Notification::None };
}

int countFromStats(const EventStats& s)
{
    return s.empty() ? -1 : int(s.notableCount);
}

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
    return findIndirect(d->unsyncedEvents, txnId, &RoomEvent::transactionId);
}

Room::PendingEvents::const_iterator Room::findPendingEvent(const QString& txnId) const
{
    return findIndirect(d->unsyncedEvents, txnId, &RoomEvent::transactionId);
}

const Room::RelatedEvents Room::relatedEvents(
    const QString& evtId, EventRelation::reltypeid_t relType) const
{
    return d->relations.value({ evtId, relType });
}

const Room::RelatedEvents Room::relatedEvents(
    const RoomEvent& evt, EventRelation::reltypeid_t relType) const
{
    return relatedEvents(evt.id(), relType);
}

const RoomCreateEvent* Room::creation() const
{
    return currentState().get<RoomCreateEvent>();
}

const RoomTombstoneEvent* Room::tombstone() const
{
    return currentState().get<RoomTombstoneEvent>();
}

void Room::Private::getAllMembers()
{
    // If already loaded or already loading, there's nothing to do here.
    if (q->joinedCount() <= currentState.eventsOfType(RoomMemberEvent::TypeId).size() || isJobPending(allMembersJob))
        return;

    allMembersJob = connection->callApi<GetMembersByRoomJob>(
        id, connection->nextBatchToken(), "join"_ls);
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

ReadReceipt Room::lastReadReceipt(const QString& userId) const
{
    return d->lastReadReceipts.value(userId);
}

ReadReceipt Room::lastLocalReadReceipt() const
{
    return d->lastReadReceipts.value(localMember().id());
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

QSet<QString> Room::userIdsAtEvent(const QString& eventId) const
{
    return d->eventIdReadUsers.value(eventId);
}

qsizetype Room::notificationCount() const
{
    return d->unreadStats.notableCount;
}

qsizetype Room::highlightCount() const { return d->serverHighlightCount; }

void Room::switchVersion(QString newVersion)
{
    if (!successorId().isEmpty()) {
        Q_ASSERT(!successorId().isEmpty());
        emit upgradeFailed(tr("The room is already upgraded"));
    }
    connection()->callApi<UpgradeRoomJob>(id(), newVersion).onFailure([this](const auto* job) {
        emit upgradeFailed(job ? job->errorString() : tr("Couldn't initiate upgrade"));
    });
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

QStringList Room::accountDataEventTypes() const
{
    QStringList events;
    events.reserve(ssize(d->accountData));
    for (const auto& [key, value] : std::as_const(d->accountData)) {
        events += key;
    }
    return events;
}

QStringList Room::tagNames() const { return d->tags.keys(); }

TagsMap Room::tags() const { return d->tags; }

Tag Room::tag(const QString& name) const { return d->tags.value(name); }

std::pair<bool, QString> validatedTag(QString name)
{
    if (name.isEmpty() || name.indexOf(u'.', 1) != -1)
        return { false, name };

    qCWarning(MAIN) << "The tag" << name
                    << "doesn't follow the CS API conventions";
    name.prepend("u."_ls);
    qCWarning(MAIN) << "Using " << name << "instead";

    return { true, name };
}

void Room::addTag(const QString& name, const Tag& tagData)
{
    const auto& checkRes = validatedTag(name);
    if (d->tags.contains(name)
        || (checkRes.first && d->tags.contains(checkRes.second)))
        return;

    emit tagsAboutToChange();
    d->tags.insert(checkRes.second, tagData);
    emit tagsChanged();
    connection()->callApi<SetRoomTagJob>(localMember().id(), id(), checkRes.second, tagData);
}

void Room::addTag(const QString& name, float order)
{
    addTag(name, Tag { order });
}

void Room::removeTag(const QString& name)
{
    if (d->tags.contains(name)) {
        emit tagsAboutToChange();
        d->tags.remove(name);
        emit tagsChanged();
        connection()->callApi<DeleteRoomTagJob>(localMember().id(), id(), name);
    } else if (!name.startsWith("u."_ls))
        removeTag("u."_ls + name);
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

    d->setTags(std::move(newTags));
    connection()->callApi<SetAccountDataPerRoomJob>(
        localMember().id(), id(), TagEvent::TypeId,
        Quotient::toJson(TagEvent::content_type { d->tags }));

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

    tags = std::move(newTags);
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

QList<RoomMember> Room::directChatMembers() const
{
    auto memberIds = connection()->directChatMemberIds(this);
    QList<RoomMember> members;
    for (const auto& memberId : memberIds) {
        if (currentState().contains<RoomMemberEvent>(memberId)) {
            members.append(RoomMember(this, currentState().get<RoomMemberEvent>(memberId)));
        }
    }
    return members;
}

QUrl Room::makeMediaUrl(const QString& eventId, const QUrl& mxcUrl) const
{
    auto url = connection()->makeMediaUrl(mxcUrl);
    QUrlQuery q(url.query());
    Q_ASSERT(q.hasQueryItem("user_id"_ls));
    q.addQueryItem("room_id"_ls, id());
    q.addQueryItem("event_id"_ls, eventId);
    url.setQuery(q);
    return url;
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

QUrl Room::urlToThumbnail(const QString& eventId) const
{
    if (auto* event = d->getEventWithFile(eventId))
        if (event->hasThumbnail()) {
            auto* thumbnail = event->content()->thumbnailInfo();
            Q_ASSERT(thumbnail != nullptr);
            return connection()->getUrlForApi<MediaThumbnailJob>(
                thumbnail->url(), thumbnail->imageSize);
        }
    qCDebug(MAIN) << "Event" << eventId << "has no thumbnail";
    return {};
}

QUrl Room::urlToDownload(const QString& eventId) const
{
    if (auto* event = d->getEventWithFile(eventId)) {
        auto* fileInfo = event->content()->fileInfo();
        Q_ASSERT(fileInfo != nullptr);
        return connection()->getUrlForApi<DownloadFileJob>(fileInfo->url());
    }
    return {};
}

QString Room::fileNameToDownload(const QString& eventId) const
{
    if (auto* event = d->getEventWithFile(eventId))
        return event->fileNameToDownload();
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

QList<RoomMember> Room::membersLeft() const {
    QList<RoomMember> members;
    members.reserve(d->membersLeft.count());
    for (const auto &memberId : d->membersLeft) {
        members.append(member(memberId));
    }
    return members;
}

int Room::timelineSize() const { return int(d->timeline.size()); }

bool Room::usesEncryption() const
{
    return !currentState()
                .queryOr(&EncryptionEvent::algorithm, QString())
                .isEmpty();
}

RoomStateView Room::currentState() const
{
    return d->currentState;
}

int Room::memberEffectivePowerLevel(const QString& memberId) const
{
    if (!successorId().isEmpty()) {
        return 0; // No one can upgrade a room that's already upgraded
    }

    const auto& mId = memberId.isEmpty() ? connection()->userId() :memberId;
    if (const auto* plEvent = currentState().get<RoomPowerLevelsEvent>()) {
        return plEvent->powerLevelForUser(mId);
    }
    if (const auto* createEvent = creation()) {
        return createEvent->senderId() == mId ? 100 : 0;
    }
    return 0; // That's rather weird but may happen, according to rvdh
}

RoomEventPtr Room::decryptMessage(const EncryptedEvent& encryptedEvent)
{
    if (const auto algorithm = encryptedEvent.algorithm();
        !isSupportedAlgorithm(algorithm)) //
    {
        qWarning(E2EE) << "Algorithm" << algorithm << "of encrypted event"
                       << encryptedEvent.id() << "is not supported";
        return {};
    }
    QString decrypted = d->groupSessionDecryptMessage(
        encryptedEvent.ciphertext(), encryptedEvent.sessionId().toLatin1(),
        encryptedEvent.id(), encryptedEvent.originTimestamp(),
        encryptedEvent.senderId());
    if (decrypted.isEmpty()) {
        // qCWarning(E2EE) << "Encrypted message is empty";
        return {};
    }
    auto decryptedEvent = encryptedEvent.createDecrypted(decrypted);
    if (decryptedEvent->roomId() == id()) {
        return decryptedEvent;
    }
    qWarning(E2EE) << "Decrypted event" << encryptedEvent.id()
                   << "not for this room; discarding";
    return {};
}

void Room::handleRoomKeyEvent(const RoomKeyEvent& roomKeyEvent,
                              const QString& senderId,
                              const QByteArray& olmSessionId,
                              const QByteArray& senderKey,
                              const QByteArray& senderEdKey)
{
    if (roomKeyEvent.algorithm() != MegolmV1AesSha2AlgoKey) {
        qCWarning(E2EE) << "Ignoring unsupported algorithm"
                        << roomKeyEvent.algorithm() << "in m.room_key event";
    }
    if (d->addInboundGroupSession(roomKeyEvent.sessionId().toLatin1(),
                                  roomKeyEvent.sessionKey(), senderId,
                                  olmSessionId, senderKey, senderEdKey)) {
        qCWarning(E2EE) << "added new inboundGroupSession:"
                        << d->groupSessions.size();
        const auto undecryptedEvents =
            d->undecryptedEvents[roomKeyEvent.sessionId()];
        for (const auto& eventId : undecryptedEvents) {
            const auto pIdx = d->eventsIndex.constFind(eventId);
            if (pIdx == d->eventsIndex.cend())
                continue;
            auto& ti = d->timeline[Timeline::size_type(*pIdx - minTimelineIndex())];
            if (auto encryptedEvent = ti.viewAs<EncryptedEvent>()) {
                if (auto decrypted = decryptMessage(*encryptedEvent)) {
                    auto&& oldEvent = eventCast<EncryptedEvent>(
                        ti.replaceEvent(std::move(decrypted)));
                    ti->setOriginalEvent(std::move(oldEvent));
                    emit replacedEvent(ti.event(), ti->originalEvent());
                    d->undecryptedEvents[roomKeyEvent.sessionId()] -= eventId;
                }
            }
        }
    }
}

int Room::joinedCount() const
{
    return d->summary.joinedMemberCount.value_or(0);
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
    if (mergeStruct(summary, newSummary, &RoomSummary::joinedMemberCount,
                    &RoomSummary::invitedMemberCount, &RoomSummary::heroes) == 0)
        return Change::None;
    qCDebug(STATE).nospace().noquote()
        << "Updated room summary for " << q->objectName() << ": " << summary;
    return Change::Summary;
}

void Room::Private::insertMemberIntoMap(const QString& memberId)
{
    const auto maybeUserName =
        currentState.query(memberId, &RoomMemberEvent::newDisplayName);
    if (!maybeUserName)
        qCDebug(MEMBERS) << "insertMemberIntoMap():" << memberId
                           << "has no name (even empty)";
    const auto userName = maybeUserName.value_or(QString());
    const auto namesakes = memberNameMap.values(userName);
    qCDebug(MEMBERS) << "insertMemberIntoMap(), user" << memberId
                     << "with name" << userName << '-'
                     << namesakes.size() << "namesake(s) found";

    // Callers should make sure they are not adding an existing user once more
    Q_ASSERT(!namesakes.contains(memberId));
    if (namesakes.contains(memberId)) { // Release version whines but continues
        qCCritical(MEMBERS) << "Trying to add a user" << memberId << "to room"
                            << q->objectName() << "but that's already in it";
        return;
    }

    // If there is exactly one namesake of the added user, signal member
    // renaming for that other one because the two should be disambiguated now
    if (namesakes.size() == 1) {
        auto otherMember = q->member(namesakes.front());
        emit q->memberNameAboutToUpdate(otherMember, otherMember.fullName());
    }
    memberNameMap.insert(userName, memberId);
    if (namesakes.size() == 1) {
        emit q->memberNameUpdated(q->member(namesakes.front()));
    }
}

void Room::Private::removeMemberFromMap(const QString& memberId)
{
    const auto userName = currentState.queryOr(memberId,
                                               &RoomMemberEvent::newDisplayName,
                                               QString());

    qCDebug(MEMBERS) << "removeMemberFromMap(), username" << userName
                     << "for user" << memberId;

    QString namesakeId{};
    auto namesakes = memberNameMap.values(userName);
    // If there was one namesake besides the removed user, signal member
    // renaming for it because it doesn't need to be disambiguated any more.
    if (namesakes.size() == 2) {
        namesakeId =
            namesakes.front() == memberId ? namesakes.back() : namesakes.front();
        Q_ASSERT_X(namesakeId != memberId, __FUNCTION__, "Room members list is broken");
        emit q->memberNameAboutToUpdate(q->member(namesakeId), userName);
    }
    if (memberNameMap.remove(userName, memberId) == 0) {
        qCDebug(MEMBERS) << "No entries removed; checking the whole list";
        // Unless at the stage of initial filling, this no removed entries
        // is suspicious; double-check that this user is not found in
        // the whole map, and stop (for debug builds) or shout in the logs
        // (for release builds) if there's one. That search is O(n), which
        // may come rather expensive for larger rooms.
        QElapsedTimer et;
        auto it = std::find(memberNameMap.cbegin(), memberNameMap.cend(), memberId);
        if (et.nsecsElapsed() > ProfilerMinNsecs / 10)
            qCDebug(MEMBERS) << "...done in" << et;
        if (it != memberNameMap.cend()) {
            // The assert (still) does more harm than good, it seems
//            Q_ASSERT_X(false, __FUNCTION__,
//                       "Mismatched name in the room members list");
            qCCritical(MEMBERS) << "Mismatched name in the room members list;"
                                   " avoiding the list corruption";
            memberNameMap.remove(it.key(), memberId);
        }
    }
    if (!namesakeId.isEmpty()) {
        emit q->memberNameUpdated(q->member(namesakeId));
    }
}

inline auto makeErrorStr(const Event& e, QByteArray msg)
{
    return msg.append("; event dump follows:\n")
        .append(QJsonDocument(e.fullJson()).toJson())
        .constData();
}

Room::Timeline::size_type
Room::Private::moveEventsToTimeline(RoomEventsRange events,
                                    EventsPlacement placement)
{
    Q_ASSERT(!events.empty());

    const auto usesEncryption = q->usesEncryption();

    // Historical messages arrive in newest-to-oldest order, so the process for
    // them is almost symmetric to the one for new messages. New messages get
    // appended from index 0; old messages go backwards from index -1.
    auto index = timeline.empty()
                     ? -((placement + 1) / 2) /* 1 -> -1; -1 -> 0 */
                     : placement == Older ? timeline.front().index()
                                          : timeline.back().index();
    auto baseIndex = index;
    for (auto&& e : events) {
        Q_ASSERT_X(e, __FUNCTION__, "Attempt to add nullptr to timeline");
        const auto eId = e->id();
        Q_ASSERT_X(
            !eId.isEmpty(), __FUNCTION__,
            makeErrorStr(*e, "Event with empty id cannot be in the timeline"));
        Q_ASSERT_X(
            !eventsIndex.contains(eId), __FUNCTION__,
            makeErrorStr(*e, "Event is already in the timeline; "
                             "incoming events were not properly deduplicated"));
        const auto& ti = placement == Older
                             ? timeline.emplace_front(std::move(e), --index)
                             : timeline.emplace_back(std::move(e), ++index);
        eventsIndex.insert(eId, index);
        if (usesEncryption)
            if (auto* const rme = ti.viewAs<RoomMessageEvent>())
                if (auto* const content = rme->content())
                    if (auto* const fileInfo = content->fileInfo())
                        if (auto* const efm = std::get_if<EncryptedFileMetadata>(
                                &fileInfo->source))
                            FileMetadataMap::add(id, eId, *efm);

        if (auto n = q->checkForNotifications(ti); n.type != Notification::None)
            notifications.insert(eId, n);
        Q_ASSERT(q->findInTimeline(eId)->event()->id() == eId);
    }
    const auto insertedSize = (index - baseIndex) * placement;
    Q_ASSERT(insertedSize == int(events.size()));
    return Timeline::size_type(insertedSize);
}

const Avatar& Room::memberAvatar(const QString& memberId) const
{
    return connection()->userAvatar(member(memberId).avatarUrl());
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
    qCDebug(MAIN) << "--- Updating room" << id() << "/" << objectName();
    bool firstUpdate = d->baseState.empty();

    if (d->prevBatch && d->prevBatch->isEmpty())
        *d->prevBatch = data.timelinePrevBatch;
    setJoinState(data.joinState);

    Changes roomChanges {};
    // The order of calculation is important - don't merge the lines!
    roomChanges |= d->updateStateFrom(std::move(data.state));
    roomChanges |= d->setSummary(std::move(data.summary));
    roomChanges |= d->addNewMessageEvents(std::move(data.timeline));

    for (auto&& ephemeralEvent : data.ephemeral)
        roomChanges |= processEphemeralEvent(std::move(ephemeralEvent));

    for (auto&& event : data.accountData)
        roomChanges |= processAccountDataEvent(std::move(event));

    roomChanges |= d->updateStatsFromSyncData(data, fromCache);

    if (roomChanges != 0) {
        // First test for changes that can only come from /sync calls and not
        // other interactions (/members, /messages etc.)
        if ((roomChanges & Change::Topic) > 0)
            emit topicChanged();

        if ((roomChanges & Change::RoomNames) > 0)
            emit namesChanged(this);

        // And now test for changes that can occur from /sync or otherwise
        d->postprocessChanges(roomChanges, !fromCache);
    }
    if (firstUpdate)
        emit baseStateLoaded();
    qCDebug(MAIN) << "--- Finished updating room" << id() << "/" << objectName();
}

void Room::Private::postprocessChanges(Changes changes, bool saveState)
{
    if (!changes)
        return;

    if ((changes & Change::Members) > 0)
        emit q->memberListChanged();

    if ((changes & (Change::RoomNames | Change::Members | Change::Summary)) > 0)
        updateDisplayname();

    if ((changes & Change::PartiallyReadStats) > 0)
        emit q->partiallyReadStatsChanged();

    if ((changes & Change::UnreadStats) > 0)
        emit q->unreadStatsChanged();

    if ((changes & Change::Highlights) > 0)
        emit q->highlightCountChanged();

    qCDebug(MAIN).nospace() << terse << changes << " = 0x" << Qt::hex
                            << uint(changes) << " in " << q->objectName();
    emit q->changed(changes);
    if (saveState)
        connection->saveRoomState(q);
}

Room::PendingEvents::iterator Room::Private::addAsPending(RoomEventPtr&& event)
{
    if (event->transactionId().isEmpty())
        event->setTransactionId(connection->generateTxnId());
    if (event->roomId().isEmpty())
        event->setRoomId(id);
    if (event->senderId().isEmpty())
        event->setSender(connection->userId());
    emit q->pendingEventAboutToAdd(std::to_address(event));
    auto it = unsyncedEvents.emplace(unsyncedEvents.end(), std::move(event));
    emit q->pendingEventAdded();
    return it;
}

const PendingEventItem& Room::Private::sendEvent(RoomEventPtr&& event)
{
    return doSendEvent(addAsPending(std::move(event)));
}

const PendingEventItem& Room::Private::doSendEvent(PendingEvents::iterator eventItemIter)
{
    Q_ASSERT(eventItemIter != unsyncedEvents.end());
    const auto& eventItem = *eventItemIter;
    const auto txnId = eventItem->transactionId();
    // TODO, #133: Enqueue the job rather than immediately trigger it.
    const RoomEvent* _event = eventItemIter->event();
    std::unique_ptr<EncryptedEvent> encryptedEvent;

    if (!q->successorId().isEmpty()) { // TODO: replace with a proper power levels check
        qCWarning(MAIN) << q << "has been upgraded, event won't be sent";
        onEventSendingFailure(eventItemIter);
        return eventItem;
    }

    if (q->usesEncryption()) {
        if (!connection->encryptionEnabled()) {
            qWarning(E2EE) << "Room" << q->objectName()
                           << "uses encryption but E2EE is switched off for"
                           << connection->objectName()
                           << "- the message won't be sent";
            onEventSendingFailure(eventItemIter);
            return eventItem;
        }
        if (!hasValidMegolmSession() || shouldRotateMegolmSession()) {
            createMegolmSession();
        }

        // Send the session to other people
        connection->sendSessionKeyToDevices(id, *currentOutboundMegolmSession,
                                            getDevicesWithoutKey());

        const auto encrypted = currentOutboundMegolmSession->encrypt(
            QJsonDocument(eventItem->fullJson()).toJson());
        currentOutboundMegolmSession->setMessageCount(
            currentOutboundMegolmSession->messageCount() + 1);
        connection->database()->saveCurrentOutboundMegolmSession(
            id, *currentOutboundMegolmSession);
        encryptedEvent = makeEvent<EncryptedEvent>(
            encrypted, connection->olmAccount()->identityKeys().curve25519,
            connection->deviceId(), QString::fromLatin1(currentOutboundMegolmSession->sessionId()));
        encryptedEvent->setTransactionId(connection->generateTxnId());
        encryptedEvent->setRoomId(id);
        encryptedEvent->setSender(connection->userId());
        if (eventItem->contentJson().contains(RelatesToKey)) {
            encryptedEvent->setRelation(eventItem->contentJson()[RelatesToKey].toObject());
        }
        // We show the unencrypted event locally while pending. The echo
        // check will throw the encrypted version out
        _event = encryptedEvent.get();
    }

    if (auto call = connection->callApi<SendMessageJob>(BackgroundRequest, id, _event->matrixType(),
                                                        txnId, _event->contentJson())) {
        // Below - find pending events by txnIds again because PendingEventItems may move around
        // as unsyncedEvents vector grows.
        Room::connect(call, &BaseJob::sentRequest, q, [this, txnId] {
            auto it = q->findPendingEvent(txnId);
            if (it == unsyncedEvents.end()) {
                qWarning(EVENTS) << "Pending event for transaction" << txnId
                                 << "not found - got synced so soon?";
                return;
            }
            it->setDeparted();
            emit q->pendingEventChanged(int(it - unsyncedEvents.begin()));
        });
        call.onResult(q, [this, txnId, call] {
            auto it = q->findPendingEvent(txnId);
            if (!call->status().good()) {
                onEventSendingFailure(it, call);
                return;
            }
            if (it != unsyncedEvents.end())
                onEventReachedServer(it, call->eventId());
            else
                qDebug(EVENTS) << "Pending event for transaction" << txnId
                               << "already merged";

            emit q->messageSent(txnId, call->eventId());
        });
    } else
        onEventSendingFailure(eventItemIter);
    return eventItem;
}

void Room::Private::onEventReachedServer(PendingEvents::iterator eventItemIter,
                                         const QString& eventId)
{
    if (QUO_ALARM(eventItemIter == unsyncedEvents.end()))
        return;

    if (eventItemIter->deliveryStatus() != EventStatus::ReachedServer) {
        eventItemIter->setReachedServer(eventId);
        emit q->pendingEventChanged(int(eventItemIter - unsyncedEvents.begin()));
    }
}

void Room::Private::onEventSendingFailure(PendingEvents::iterator eventItemIter, const BaseJob* call)
{
    Q_ASSERT(eventItemIter != unsyncedEvents.end());
    if (eventItemIter == unsyncedEvents.end()) // ¯\_(ツ)_/¯
        return;

    eventItemIter->setSendingFailed(call ? call->statusCaption() % ": "_ls % call->errorString()
                                         : tr("The call could not be started"));
    emit q->pendingEventChanged(int(eventItemIter - unsyncedEvents.begin()));
}

PendingEventItem::future_type Room::whenMessageMerged(QString txnId) const
{
    if (auto it = findPendingEvent(txnId); it != d->unsyncedEvents.cend())
        return it->whenMerged();
    return {};
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
    return d->doSendEvent(it)->transactionId();
}

// Using a function defers actual tr() invocation to the moment when
// translations are initialised
auto FileTransferCancelledMsg() { return Room::tr("File transfer cancelled"); }

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
    return post<RoomMessageEvent>(plainText, type)->transactionId();
}

QString Room::postPlainText(const QString& plainText)
{
    return postMessage(plainText, MessageEventType::Text);
}

QString Room::postHtmlMessage(const QString& plainText, const QString& html,
                              MessageEventType type)
{
    return post<RoomMessageEvent>(
               plainText, type, new EventContent::TextContent(html, QStringLiteral("text/html")))
        ->transactionId();
}

QString Room::postHtmlText(const QString& plainText, const QString& html)
{
    return postHtmlMessage(plainText, html);
}

QString Room::postReaction(const QString& eventId, const QString& key)
{
    return post<ReactionEvent>(eventId, key)->transactionId();
}

QString Room::Private::doPostFile(RoomEventPtr&& msgEvent, const QUrl& localUrl)
{
    const auto txnId = addAsPending(std::move(msgEvent))->event()->transactionId();
    // Remote URL will only be known after upload; fill in the local path
    // to enable the preview while the event is pending.
    q->uploadFile(txnId, localUrl);
    // Below, the upload job is used as a context object to clean up connections
    const auto& transferJob = fileTransfers.value(txnId).job;
    connect(q, &Room::fileTransferCompleted, transferJob,
        [this, txnId](const QString& tId, const QUrl&,
                      const FileSourceInfo& fileMetadata) {
            if (tId != txnId)
                return;

            const auto it = q->findPendingEvent(txnId);
            if (it != unsyncedEvents.end()) {
                it->setFileUploaded(fileMetadata);
                emit q->pendingEventChanged(int(it - unsyncedEvents.begin()));
                doSendEvent(it);
            } else {
                // Normally in this situation we should instruct
                // the media server to delete the file; alas, there's no
                // API specced for that.
                qCWarning(MAIN)
                    << "File uploaded to" << getUrlFromSourceInfo(fileMetadata)
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
    // This is required because toLocalFile doesn't work on android and toString doesn't work on the desktop
    auto url = fileInfo->url().isLocalFile() ? fileInfo->url().toLocalFile() : fileInfo->url().toString();
    QFileInfo localFile { url };
    Q_ASSERT(localFile.isFile());

    return d->doPostFile(
        makeEvent<RoomMessageEvent>(
            plainText, RoomMessageEvent::rawMsgTypeForFile(localFile), content),
        fileInfo->url());
}

QString Room::postEvent(RoomEvent* event)
{
    return d->sendEvent(RoomEventPtr(event))->transactionId();
}

const PendingEventItem& Room::post(RoomEventPtr event)
{
    return d->sendEvent(std::move(event));
}

QString Room::postJson(const QString& matrixType, const QJsonObject& eventContent)
{
    return d->sendEvent(loadEvent<RoomEvent>(matrixType, eventContent))->transactionId();
}

SetRoomStateWithKeyJob* Room::setState(const StateEvent& evt)
{
    return setState(evt.matrixType(), evt.stateKey(), evt.contentJson());
}

SetRoomStateWithKeyJob* Room::setState(const QString& evtType,
                                       const QString& stateKey,
                                       const QJsonObject& contentJson)
{
    return d->requestSetState(evtType, stateKey, contentJson);
}

void Room::setName(const QString& newName)
{
    setState<RoomNameEvent>(newName);
}

void Room::setCanonicalAlias(const QString& newAlias)
{
    setState<RoomCanonicalAliasEvent>(newAlias, altAliases());
}

void Room::setPinnedEvents(const QStringList& events)
{
    setState<RoomPinnedEventsEvent>(events);
}
void Room::setLocalAliases(const QStringList& aliases)
{
    setState<RoomCanonicalAliasEvent>(canonicalAlias(), aliases);
}

void Room::setTopic(const QString& newTopic)
{
    setState<RoomTopicEvent>(newTopic);
}

bool isEchoEvent(const RoomEventPtr& le, const PendingEventItem& re)
{
    if (le->metaType() != re->metaType())
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
    post<CallInviteEvent>(callId, lifetime, sdp);
}

void Room::sendCallCandidates(const QString& callId,
                              const QJsonArray& candidates)
{
    Q_ASSERT(supportsCalls());
    post<CallCandidatesEvent>(callId, candidates);
}

void Room::answerCall(const QString& callId, const QString& sdp)
{
    Q_ASSERT(supportsCalls());
    post<CallAnswerEvent>(callId, sdp);
}

void Room::hangupCall(const QString& callId)
{
    Q_ASSERT(supportsCalls());
    post<CallHangupEvent>(callId);
}

JobHandle<GetRoomEventsJob> Room::getPreviousContent(int limit, const QString& filter)
{
    if (!d->isLoadingMessages) {
        d->isLoadingMessages = true;
        auto count = d->loadFromDb(limit);
        d->isLoadingMessages = false;
        if (count > 0) {
            return {};
        }
        return d->getPreviousContent(limit, filter);
    }
    return {};
}

JobHandle<GetRoomEventsJob> Room::Private::getPreviousContent(int limit, const QString& filter)
{
    if (!prevBatch)
        return {}; // No further history = cancelled future

    if (isJobPending(eventsHistoryJob))
        return eventsHistoryJob;

    lastRequestedHistorySize = limit;
    eventsHistoryJob =
        connection->callApi<GetRoomEventsJob>(id, "b"_ls, *prevBatch, QString(), limit, filter);
    emit q->eventsHistoryJobChanged();
    connect(eventsHistoryJob, &BaseJob::success, q, [this] {
        if (const auto newPrevBatch = eventsHistoryJob->end();
            !newPrevBatch.isEmpty() && *prevBatch != newPrevBatch) //
        {
            *prevBatch = newPrevBatch;
            auto query = connection->database()->prepareQuery("DELETE FROM event_batch_tokens WHERE roomId=:roomId;"_ls);
            query.bindValue(":roomId"_ls, q->id());
            connection->database()->execute(query);
            query = connection->database()->prepareQuery("INSERT INTO event_batch_tokens(roomId, token) VALUES(:roomId, :token);"_ls);
            query.bindValue(":roomId"_ls, q->id());
            query.bindValue(":token"_ls, newPrevBatch);
            connection->database()->execute(query);
        } else {
            qCDebug(MESSAGES)
                << "Room" << q->objectName() << "has loaded all history";
            prevBatch.reset();
        }

        auto [changes, from] = addHistoricalMessageEvents(eventsHistoryJob->chunk());
        // The following condition will only trigger once, next time getPreviousContent()
        // will return without spawning GetRoomEventsJob
        if (!prevBatch)
            emit q->allHistoryLoadedChanged();
        changes |= updateStats(from, historyEdge());
        if (changes > 0)
            postprocessChanges(changes);
    });
    connect(eventsHistoryJob, &QObject::destroyed, q,
            &Room::eventsHistoryJobChanged);
    return eventsHistoryJob;
}

void Room::inviteToRoom(const QString& memberId)
{
    connection()->callApi<InviteUserJob>(id(), memberId);
}

JobHandle<LeaveRoomJob> Room::leaveRoom()
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
    connection()->callApi<RedactEventJob>(id(), eventId,
                                          connection()->generateTxnId(), reason);
}

void Room::uploadFile(const QString& id, const QUrl& localFilename,
                      const QString& overrideContentType)
{
    // This is required because toLocalFile doesn't work on android and toString doesn't work on the desktop
    auto fileName = localFilename.isLocalFile() ? localFilename.toLocalFile() : localFilename.toString();
    FileSourceInfo fileMetadata;
    QTemporaryFile tempFile;
    if (usesEncryption()) {
        tempFile.open();
        QFile file(fileName);
        file.open(QFile::ReadOnly);
        QByteArray data;
        std::tie(fileMetadata, data) = encryptFile(file.readAll());
        tempFile.write(data);
        tempFile.close();
        fileName = QFileInfo(tempFile).absoluteFilePath();
    }
    auto job = connection()->uploadFile(fileName, overrideContentType);
    if (isJobPending(job)) {
        d->fileTransfers[id] = { job, fileName, true };
        connect(job, &BaseJob::uploadProgress, this,
                [this, id](qint64 sent, qint64 total) {
                    d->fileTransfers[id].update(sent, total);
                    emit fileTransferProgress(id, sent, total);
                });
        connect(job, &BaseJob::success, this,
                [this, id, localFilename, job, fileMetadata]() mutable {
                    // The lambda is mutable to change encryptedFileMetadata
                    d->fileTransfers[id].status = FileTransferInfo::Completed;
                    setUrlInSourceInfo(fileMetadata, QUrl(job->contentUri()));
                    emit fileTransferCompleted(id, localFilename, fileMetadata);
                });
        connect(job, &BaseJob::failure, this,
                [this, id, job] { d->failedTransfer(id, job->errorString()); });
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
    const auto fileUrl = fileInfo->url();
    auto filePath = localFilename.toLocalFile();
    if (filePath.isEmpty()) { // Setup default file path
        filePath = fileInfo->url().path().mid(1) % u'_' % event->fileNameToDownload();

        if (filePath.size() > 200) // If too long, elide in the middle
            filePath.replace(128, filePath.size() - 192, "---"_ls);

        filePath = QDir::tempPath() % u'/' % filePath;
        qDebug(MAIN) << "File path:" << filePath;
    }
    DownloadFileJob *job = nullptr;
    if (auto* fileMetadata =
            std::get_if<EncryptedFileMetadata>(&fileInfo->source)) {
        job = connection()->downloadFile(fileUrl, *fileMetadata, filePath);
    } else {
        job = connection()->downloadFile(fileUrl, filePath);
    }
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
        emit newFileTransfer(eventId, localFilename);
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

void Room::Private::dropExtraneousEvents(RoomEvents& events) const
{
    if (events.empty())
        return;

    // Multiple-remove (by different criteria), single-erase
    // 1. Check for duplicates against the timeline and for events from ignored
    //    users
    auto newEnd =
        remove_if(events.begin(), events.end(), [this](const RoomEventPtr& e) {
            return eventsIndex.contains(e->id())
                   || connection->isIgnored(e->senderId());
        });

    // 2. Check for duplicates within the batch if there are still events.
    for (auto eIt = events.begin(); distance(eIt, newEnd) > 1; ++eIt)
        newEnd = remove_if(eIt + 1, newEnd, [eIt](const RoomEventPtr& e) {
            return e->id() == (*eIt)->id();
        });

    if (newEnd == events.end())
        return;

    qCDebug(EVENTS) << "Dropping" << distance(newEnd, events.end())
                    << "extraneous event(s)";
    events.erase(newEnd, events.end());
}

void Room::Private::decryptIncomingEvents(RoomEvents& events)
{
    if (!connection->encryptionEnabled())
        return;
    if (!q->usesEncryption())
        return; // If the room doesn't use encryption now, it never did

    QElapsedTimer et;
    et.start();
    size_t totalDecrypted = 0;
    for (auto& eptr : events) {
        if (eptr->isRedacted())
            continue;
        if (const auto& eeptr = eventCast<EncryptedEvent>(eptr)) {
            if (auto decrypted = q->decryptMessage(*eeptr)) {
                ++totalDecrypted;
                auto&& oldEvent = eventCast<EncryptedEvent>(
                        std::exchange(eptr, std::move(decrypted)));
                eptr->setOriginalEvent(std::move(oldEvent));
            } else
                undecryptedEvents[eeptr->sessionId()] += eeptr->id();
        }
    }
    if (totalDecrypted > 5 || et.nsecsElapsed() >= ProfilerMinNsecs)
        qDebug(PROFILER)
            << "Decrypted" << totalDecrypted << "events in" << et;
}

//! \brief Make a redacted event
//!
//! This applies the redaction procedure as defined by the CS API specification
//! to the event's JSON and returns the resulting new event. It is
//! the responsibility of the caller to dispose of the original event after that.
RoomEventPtr makeRedacted(const RoomEvent& target,
                          const RedactionEvent& redaction)
{
    // The logic below faithfully follows the spec despite quite a few of
    // the preserved keys being only relevant for homeservers. Just in case.
    static const QStringList TopLevelKeysToKeep{
        EventIdKey,  TypeKey,          RoomIdKey,        SenderKey,
        StateKeyKey, ContentKey,       "hashes"_ls,      "signatures"_ls,
        "depth"_ls,  "prev_events"_ls, "auth_events"_ls, "origin_server_ts"_ls
    };

    auto originalJson = target.fullJson();
    for (auto it = originalJson.begin(); it != originalJson.end();) {
        if (!TopLevelKeysToKeep.contains(it.key()))
            it = originalJson.erase(it);
        else
            ++it;
    }
    if (!target.is<RoomCreateEvent>()) { // See MSC2176 on create events
        static const QHash<QString, QStringList> ContentKeysToKeepPerType{
            { RedactionEvent::TypeId, { "redacts"_ls } },
            { RoomMemberEvent::TypeId,
              { "membership"_ls, "join_authorised_via_users_server"_ls } },
            { RoomPowerLevelsEvent::TypeId,
              { "ban"_ls, "events"_ls, "events_default"_ls, "invite"_ls,
                "kick"_ls, "redact"_ls, "state_default"_ls, "users"_ls,
                "users_default"_ls } },
            // TODO: Replace with RoomJoinRules::TypeId etc. once available
            { "m.room.join_rules"_ls, { "join_rule"_ls, "allow"_ls } },
            { "m.room.history_visibility"_ls, { "history_visibility"_ls } }
        };

        if (const auto contentKeysToKeep = ContentKeysToKeepPerType.value(target.matrixType());
            !contentKeysToKeep.isEmpty()) //
        {
            editSubobject(originalJson, ContentKey, [&contentKeysToKeep](QJsonObject& content) {
                for (auto it = content.begin(); it != content.end();) {
                    if (!contentKeysToKeep.contains(it.key()))
                        it = content.erase(it);
                    else
                        ++it;
                }
            });
        } else {
            originalJson.remove(ContentKey);
            originalJson.remove(PrevContentKey);
        }
    }
    replaceSubvalue(originalJson, UnsignedKey, RedactedCauseKey, redaction.fullJson());

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
    if (ti->is<RoomMessageEvent>())
        FileMetadataMap::remove(id, ti->id());

    // Make a new event from the redacted JSON and put it in the timeline
    // instead of the redacted one. oldEvent will be deleted on return.
    auto oldEvent = ti.replaceEvent(makeRedacted(*ti, redaction));
    qCDebug(EVENTS) << "Redacted" << oldEvent->id() << "with" << redaction.id();
    if (oldEvent->isStateEvent()) {
        // Check whether the old event was a part of current state; if it was,
        // update the current state to the redacted event object.
        const auto currentStateEvt =
            currentState.get(oldEvent->matrixType(), oldEvent->stateKey());
        Q_ASSERT(currentStateEvt);
        if (currentStateEvt == oldEvent.get()) {
            // Historical states can't be in currentState
            Q_ASSERT(ti.index() >= 0);
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
        const auto& content = reaction->content().value;
        const std::pair lookupKey { content.eventId, content.type };
        if (relations.contains(lookupKey)) {
            relations[lookupKey].removeOne(reaction);
            emit q->updatedEvent(content.eventId);
        }
    }
    q->onRedaction(*oldEvent, *ti);
    emit q->replacedEvent(ti.event(), std::to_address(oldEvent));
    // By now, all references to oldEvent must have been updated to ti.event()
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
    auto newContent = replacement.contentPart<QJsonObject>("m.new_content"_ls);
    addParam<IfNotEmpty>(newContent, RelatesToKey, target.contentPart<QJsonObject>(RelatesToKey));
    auto originalJson = target.fullJson();
    originalJson[ContentKey] = newContent;
    editSubobject(originalJson, UnsignedKey, [&replacement](QJsonObject& unsignedData) {
        replaceSubvalue(unsignedData, "m.relations"_ls, "m.replace"_ls, replacement.id());
    });

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
    const auto* const rme = ti.viewAs<RoomMessageEvent>();
    if (!rme) {
        qCWarning(STATE) << "Ignoring attempt to replace a non-message event"
                         << ti->id();
        return false;
    }
    if (rme->replacedBy() == newEvent.id()) {
        qCDebug(STATE) << "Event" << ti->id() << "is already replaced with"
                       << newEvent.id();
        return true;
    }

    // Make a new event from the redacted JSON and put it in the timeline
    // instead of the redacted one. oldEvent will be deleted on return.
    auto oldEvent = ti.replaceEvent(makeReplaced(*ti, newEvent));
    qCDebug(STATE) << "Replaced" << oldEvent->id() << "with" << newEvent.id();
    emit q->replacedEvent(ti.event(), std::to_address(oldEvent));
    return true;
}

Connection* Room::connection() const
{
    Q_ASSERT(d->connection);
    return d->connection;
}

void Room::Private::addRelation(const ReactionEvent& reactionEvt)
{
    const auto& content = reactionEvt.content().value;
    // See ReactionEvent::isValid()
    Q_ASSERT(content.type == EventRelation::AnnotationType);
    const auto isSameReaction = [&reactionEvt](const RoomEvent* existingEvent) {
        const auto* reactionEvt2 = eventCast<const ReactionEvent>(existingEvent);
        const auto& r1 = reactionEvt.content().value;
        const auto& r2 = reactionEvt2->content().value;
        return reactionEvt2 != nullptr
               && reactionEvt.senderId() == reactionEvt2->senderId()
               && r1.eventId == r2.eventId && r1.key == r2.key;
    };

    auto& thisEventReactions = relations[{ content.eventId, content.type }];
    if (std::any_of(thisEventReactions.cbegin(), thisEventReactions.cend(),
                    isSameReaction)) {
        qDebug(MESSAGES) << "Skipping a duplicate reaction from"
                         << reactionEvt.senderId();
        return;
    }
    thisEventReactions << &reactionEvt;
    emit q->updatedEvent(content.eventId);
}

/// Whether the event is a redaction or a replacement
inline bool isEditing(const RoomEventPtr& ep)
{
    Q_ASSERT(ep);
    return ep->switchOnType([](const RedactionEvent&) { return true; },
                     [](const RoomMessageEvent& rme) {
                         return !rme.replacedEvent().isEmpty();
                     },
                     false);
}

Room::Timeline::size_type Room::Private::mergePendingEvent(PendingEvents::iterator localEchoIt,
                                                           RoomEvents::iterator remoteEchoIt)
{
    auto* remoteEcho = remoteEchoIt->get();
    const auto pendingEvtIdx = int(localEchoIt - unsyncedEvents.begin());
    onEventReachedServer(localEchoIt, remoteEcho->id());
    emit q->pendingEventAboutToMerge(remoteEcho, pendingEvtIdx);
    qCDebug(MESSAGES) << "Merging pending event from transaction" << remoteEcho->transactionId()
                      << "into" << remoteEcho->id();
    auto transfer = fileTransfers.take(remoteEcho->transactionId());
    if (transfer.status != FileTransferInfo::None)
        fileTransfers.insert(remoteEcho->id(), transfer);
    // After emitting pendingEventAboutToMerge() above we cannot rely
    // on the previously obtained localEcho staying valid
    // because a signal handler may send another message, thereby altering
    // unsyncedEvents (see #286). Fortunately, unsyncedEvents only grows at
    // its back so we can rely on the index staying valid at least.
    localEchoIt = unsyncedEvents.begin() + pendingEvtIdx;
    localEchoIt->setMerged(*remoteEcho);
    unsyncedEvents.erase(localEchoIt);
    const auto insertedSize = moveEventsToTimeline({ remoteEchoIt, remoteEchoIt + 1 }, Newer);
    if (insertedSize > 0)
        q->onAddNewTimelineEvents(syncEdge() - insertedSize);

    emit q->pendingEventMerged();
    return insertedSize;
}

Room::Changes Room::Private::addNewMessageEvents(RoomEvents&& events)
{
    dropExtraneousEvents(events);
    if (events.empty())
        return Change::None;

    storeInDb(events);

    decryptIncomingEvents(events);

    QElapsedTimer et;
    et.start();

    {
        // Pre-process redactions and edits so that events that get
        // redacted/replaced in the same batch landed in the timeline already
        // treated.
        // NB: We have to store redacting/replacing events to the timeline too -
        // see #220.
        auto it = std::find_if(events.begin(), events.end(), isEditing);
        for (const auto& eptr : std::ranges::subrange(it, events.end())) {
            if (auto* r = eventCast<RedactionEvent>(eptr)) {
                // Try to find the target in the timeline, then in the batch.
                if (processRedaction(*r))
                    continue;
                if (auto targetIt = findIndirect(events, r->redactedEvent(), &RoomEvent::id);
                    targetIt != events.end())
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
                if (auto targetIt =
                        findIndirect(events.begin(), it, msg->replacedEvent(), &RoomEvent::id);
                    targetIt != it)
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
        const auto& [remoteEcho, localEcho] = findFirstOf(it, events.end(), unsyncedEvents.begin(),
                                                          unsyncedEvents.end(), isEchoEvent);

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
        totalInserted += mergePendingEvent(localEcho, remoteEcho);
    }
    // Events merged and transferred from `events` to `timeline` now.
    const auto from = syncEdge() - totalInserted;

    if (q->supportsCalls())
        for (auto it = from; it != syncEdge(); ++it)
            if (const auto* evt = it->viewAs<CallEvent>())
                emit q->callEvent(q, evt);

    for (auto it = from; it != syncEdge(); ++it) {
        if (it->event()->senderId() == connection->userId()) {
            if (const auto* evt = it->viewAs<RoomMessageEvent>()) {
                if (evt->rawMsgtype() == "m.key.verification.request"_ls && pendingKeyVerificationSession && evt->senderId() == q->localMember().id()) {
                    keyVerificationSessions[evt->id()] = pendingKeyVerificationSession;
                    connect(pendingKeyVerificationSession.get(), &QObject::destroyed, q, [this, evt] {
                        keyVerificationSessions.remove(evt->id());
                    });
                    pendingKeyVerificationSession->setRequestEventId(evt->id());
                    pendingKeyVerificationSession.clear();
                }
            }
            continue;
        }
        if (const auto* evt = it->viewAs<RoomMessageEvent>()) {
            if (evt->rawMsgtype() == "m.key.verification.request"_ls) {
                if (evt->originTimestamp() > QDateTime::currentDateTime().addSecs(-60)) {
                    auto session = new KeyVerificationSession(evt, q);
                    emit connection->newKeyVerificationSession(session);
                    keyVerificationSessions[evt->id()] = session;
                    connect(session, &QObject::destroyed, q, [this, evt] {
                        keyVerificationSessions.remove(evt->id());
                    });
                }
            }
        }
        if (auto event = it->viewAs<KeyVerificationEvent>()) {
            const auto &baseEvent = event->contentJson()["m.relates_to"_ls]["event_id"_ls].toString();
            if (event->matrixType() == "m.key.verification.done"_ls) {
                continue;
            }
            if (keyVerificationSessions.contains(baseEvent)) {
                keyVerificationSessions[baseEvent]->handleEvent(*event);
            } else
                qCWarning(E2EE) << "Unknown verification session, id" << baseEvent;
        }
    }

    if (totalInserted > 0) {
        addRelations(from, syncEdge());

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
    if (totalInserted > 9 || et.nsecsElapsed() >= ProfilerMinNsecs)
        qCDebug(PROFILER) << "Added" << totalInserted << "new event(s) to"
                          << q->objectName() << "in" << et;
    return roomChanges;
}

void Room::Private::storeInDb(const RoomEvents &events)
{
    for (const auto& eptr : events) {
        const auto& e = *eptr;
        connection->database()->transaction();

        auto query = connection->database()->prepareQuery(QStringLiteral("INSERT INTO events(roomId, ts, json) VALUES(:roomId, :ts, :json);"));
        query.bindValue(QStringLiteral(":roomId"), q->id());
        query.bindValue(QStringLiteral(":ts"), e.originTimestamp().toMSecsSinceEpoch());
        query.bindValue(QStringLiteral(":json"), QString::fromUtf8(QJsonDocument(e.fullJson()).toJson(QJsonDocument::Compact)));
        connection->database()->execute(query);

        connection->database()->commit();
    }
}

int Room::Private::loadFromDb(int limit)
{
    auto query = connection->database()->prepareQuery(QStringLiteral("SELECT json, ts FROM events WHERE roomId=:roomId AND ts <= :lastTs ORDER BY ts DESC LIMIT :limit"));
    query.bindValue(QStringLiteral(":roomId"), id);
    query.bindValue(QStringLiteral(":lastTs"), lastTs);
    query.bindValue(QStringLiteral(":limit"), limit);
    connection->database()->execute(query);

    RoomEvents events;
    while(query.next()) {
        events.push_back(loadEvent<RoomEvent>(QJsonDocument::fromJson(query.value("json"_ls).toString().toUtf8()).object()));
        lastTs = query.value("ts"_ls).toLongLong();

    }
    addHistoricalMessageEvents(std::move(events), true);
    return events.size();
}

std::pair<Room::Changes, Room::rev_iter_t> Room::Private::addHistoricalMessageEvents(RoomEvents&& events, bool fromDb)
{
    dropExtraneousEvents(events);
    if (events.empty())
        return { Change::None, historyEdge() };

    if (!fromDb) {
        storeInDb(events);
    }

    const auto timelineSize = timeline.size();
    decryptIncomingEvents(events);

    QElapsedTimer et;
    et.start();
    Changes changes {};
    // In case of lazy-loading new members may be loaded with historical
    // messages. Also, the cache doesn't store events with empty content;
    // so when such events show up in the timeline they should be properly
    // incorporated.
    for (const auto& eptr : events) {
        const auto& e = *eptr;
        if (e.isStateEvent()
            && !currentState.contains(e.matrixType(), e.stateKey())) {
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

    addRelations(from, historyEdge());
    Q_ASSERT(timeline.size() == timelineSize + insertedSize);
    if (insertedSize > 9 || et.nsecsElapsed() >= ProfilerMinNsecs)
        qCDebug(PROFILER) << "Added" << insertedSize << "historical event(s) to" << q->objectName()
                          << "in" << et;
    return { changes, from };
}

void Room::Private::preprocessStateEvent(const RoomEvent& newEvent,
                                         const RoomEvent* curEvent)
{
    newEvent.switchOnType(
        [this, curEvent](const RoomMemberEvent& rme) {
            switch (const auto prevMembership =
                        lift(&RoomMemberEvent::membership,
                             eventCast<const RoomMemberEvent>(curEvent))
                            .value_or(Membership::Leave)) {
            case Membership::Invite:
                if (rme.membership() != prevMembership) {
                    membersInvited.removeOne(rme.userId());
                    Q_ASSERT(!membersInvited.contains(rme.userId()));
                }
                break;
            case Membership::Join: {
                if (rme.membership() == Membership::Join) {
                    // rename/avatar change or no-op
                    if (rme.newDisplayName()) {
                        emit q->memberNameAboutToUpdate(q->member(rme.userId()), *rme.newDisplayName());
                        removeMemberFromMap(rme.userId());
                    }
                    if (!rme.newDisplayName() && !rme.newAvatarUrl())
                        qCDebug(MEMBERS).nospace().noquote()
                            << "No-op membership event for " << rme.userId()
                            << ": " << rme;
                } else {
                    if (rme.membership() == Membership::Invite)
                        qCWarning(MAIN)
                            << "Membership change from Join to Invite:" << rme;
                    // whatever the new membership, it's no more Join
                    removeMemberFromMap(rme.userId());
                    emit q->memberLeft(q->member(rme.userId()));
                }
                break;
            }
            case Membership::Ban:
            case Membership::Knock:
            case Membership::Leave:
                if (rme.membership() == Membership::Invite
                    || rme.membership() == Membership::Join) {
                    membersLeft.removeOne(rme.userId());
                    Q_ASSERT(!membersLeft.contains(rme.userId()));
                }
                break;
            case Membership::Undefined:
                ; // A warning will be dropped in Room::P::processStateEvent()
            }
        },
        [this, curEvent](const EncryptionEvent& ee) {
            if (curEvent)
                qCWarning(STATE) << "Room" << q->objectName()
                                 << "is already encrypted but a new room "
                                    "encryption event arrived";
            if (ee.algorithm().isEmpty())
                qWarning(STATE)
                    << "The encryption event for room" << q->objectName()
                    << "doesn't have 'algorithm' specified";

        });
}

Room::Changes Room::processStateEvent(const RoomEvent& e)
{
    if (!e.isStateEvent())
        return Change::None;

    // Find a value (create an empty one if necessary) and get a reference
    // to it, anticipating a change further in the function.
    auto& curStateEvent = d->currentState[{ e.matrixType(), e.stateKey() }];

    d->preprocessStateEvent(e, curStateEvent);

    // Change the state
    const auto* const oldStateEvent =
        std::exchange(curStateEvent, static_cast<const StateEvent*>(&e));
    Q_ASSERT(!oldStateEvent
             || (oldStateEvent->matrixType() == e.matrixType()
                 && oldStateEvent->stateKey() == e.stateKey()));
    if (is<RoomMemberEvent>(e))
        qCDebug(MEMBERS) << "Updated room member state:" << e;
    else
        qCDebug(STATE) << "Updated room state:" << e;

    const auto result = d->processStateEvent(*curStateEvent, oldStateEvent);

    Q_ASSERT(result != Change::None);
    // Whatever the outcome, the relevant piece of state should stay valid
    // (the absense of event is a valid state, too)
    Q_ASSERT(currentState().queryOr(e.matrixType(), e.stateKey(),
                                    &Event::isStateEvent, true));
    return result;
}

//! Update internal structures as per the change and work out the return value
Room::Change Room::Private::processStateEvent(const RoomEvent& curEvent,
                                              const RoomEvent* oldEvent)
{
    return curEvent.switchOnType(
        [](const RoomNameEvent&) { return Change::RoomNames; },
        [this, oldEvent](const RoomCanonicalAliasEvent& cae) {
            q->setObjectName(cae.alias().isEmpty() ? id : cae.alias());
            QStringList previousAltAliases{};
            if (const auto* oldCae =
                    static_cast<const RoomCanonicalAliasEvent*>(oldEvent)) {
                previousAltAliases = oldCae->altAliases();
                if (!oldCae->alias().isEmpty())
                    previousAltAliases.push_back(oldCae->alias());
            }

            auto newAliases = cae.altAliases();
            if (!cae.alias().isEmpty())
                newAliases.push_front(cae.alias());

            connection->updateRoomAliases(id, previousAltAliases, newAliases);
            return Change::RoomNames;
        },
        [this](const RoomPinnedEventsEvent&) {
            emit q->pinnedEventsChanged();
            return Change::Other;
        },
        [](const RoomTopicEvent&) { return Change::Topic; },
        [this](const RoomAvatarEvent& evt) {
            if (avatar.updateUrl(evt.url()))
                emit q->avatarChanged();
            return Change::Avatar;
        },
        [this, oldEvent](const RoomMemberEvent& evt) {
            // See also Room::P::preprocessStateEvent()
            const auto prevMembership =
                lift(&RoomMemberEvent::membership,
                        static_cast<const RoomMemberEvent*>(oldEvent))
                    .value_or(Membership::Leave);
            switch (evt.membership()) {
            case Membership::Join: {
                if (prevMembership != Membership::Join) {
                    insertMemberIntoMap(evt.userId());
                    emit q->memberJoined(q->member(evt.userId()));
                } else {
                    if (evt.newDisplayName()) {
                        insertMemberIntoMap(evt.userId());
                        emit q->memberNameUpdated(q->member(evt.userId()));
                    }
                    if (evt.newAvatarUrl()) {
                        emit q->memberAvatarUpdated(q->member(evt.userId()));
                    }
                }
                break;
            }
            case Membership::Invite:
                if (!membersInvited.contains(evt.userId()))
                        membersInvited.push_back(evt.userId());
                if (evt.userId() == connection->userId() && evt.isDirect())
                    connection->addToDirectChats(q, evt.userId());
                break;
            case Membership::Knock:
            case Membership::Ban:
            case Membership::Leave:
                if (!membersLeft.contains(evt.userId()))
                    membersLeft.append(evt.userId());
                break;
            case Membership::Undefined:
                qCWarning(MEMBERS) << "Ignored undefined membership type";
            }
            return Change::Members;
        },
        [this](const EncryptionEvent&) {
            // As encryption can only be switched on once, emit the signal here
            // instead of aggregating and emitting in updateData()
            emit q->encryption();
            return Change::Other;
        },
        [this](const RoomTombstoneEvent& evt) {
            const auto successorId = evt.successorRoomId();
            if (auto* successor = connection->room(successorId))
                emit q->upgraded(evt.serverMessage(), successor);
            else
                connectUntil(connection, &Connection::loadedRoomState, q,
                    [this,successorId,serverMsg=evt.serverMessage()]
                    (Room* newRoom) {
                        if (newRoom->id() != successorId)
                            return false;
                        emit q->upgraded(serverMsg, newRoom);
                        return true;
                    });

            return Change::Other;
        },
        Change::Other);
}

Room::Changes Room::processEphemeralEvent(EventPtr&& event)
{
    Changes changes {};
    QElapsedTimer et;
    et.start();
    switchOnType(*event,
        [this, &et](const TypingEvent& evt) {
            const auto& users = evt.users();
            d->membersTyping.clear();
            d->membersTyping.reserve(users.size()); // Assume all are members
            for (const auto& userId : users)
                if (isMember(userId))
                    d->membersTyping.append(userId);

            if (d->membersTyping.size() > 3
                || et.nsecsElapsed() >= ProfilerMinNsecs)
                qDebug(PROFILER)
                    << "Processing typing events from" << users.size()
                    << "user(s) in" << objectName() << "took" << et;
            emit typingChanged();
        },
        [this, &changes, &et](const ReceiptEvent& evt) {
            const auto& receiptsJson = evt.contentJson();
            QVector<QString> updatedUserIds;
            // Most often (especially for bigger batches), receipts are
            // scattered across events (an anecdotal evidence showed 1.2-1.3
            // receipts per event on average).
            updatedUserIds.reserve(receiptsJson.size() * 2);
            for (auto eventIt = receiptsJson.begin();
                 eventIt != receiptsJson.end(); ++eventIt) {
                const auto evtId = eventIt.key();
                const auto newMarker = findInTimeline(evtId);
                if (newMarker == historyEdge())
                    qDebug(EPHEMERAL)
                        << "Event" << evtId
                        << "is not found; saving read receipt(s) anyway";
                const auto reads =
                    eventIt.value().toObject().value("m.read"_ls).toObject();
                for (auto userIt = reads.begin(); userIt != reads.end();
                     ++userIt) {
                    ReadReceipt rr{ evtId,
                                    fromJson<QDateTime>(
                                        userIt->toObject().value("ts"_ls)) };
                    const auto userId = userIt.key();
                    if (userId == connection()->userId()) {
                        // Local user is special, and will get a signal about
                        // its read receipt separately from (and before) a
                        // signal on everybody else. No particular reason, just
                        // less cumbersome code.
                        changes |= d->setLocalLastReadReceipt(newMarker, rr);
                    } else if (d->setLastReadReceipt(userId, newMarker, rr)) {
                        changes |= Change::Other;
                        updatedUserIds.push_back(userId);
                    }
                }
            }
            if (updatedUserIds.size() > 10
                || et.nsecsElapsed() >= ProfilerMinNsecs)
                qDebug(PROFILER)
                    << "Processing" << updatedUserIds.size()
                    << "non-local receipt(s) on" << receiptsJson.size()
                    << "event(s) in" << objectName() << "took" << et;
            if (!updatedUserIds.empty())
                emit lastReadEventChanged(updatedUserIds);
        });
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
        changes |= d->setFullyReadMarker(evt->eventId());

    // For all account data events
    auto& currentData = d->accountData[event->matrixType()];
    // A polymorphic event-specific comparison might be a bit more
    // efficient; maaybe do it another day
    if (!currentData || currentData->contentJson() != event->contentJson()) {
        emit accountDataAboutToChange(event->matrixType());
        currentData = std::move(event);
        qCDebug(STATE) << "Updated account data of type"
                       << currentData->matrixType();
        emit accountDataChanged(currentData->matrixType());
        changes |= Change::Other;
    }
    return changes;
}

Room::Private::users_shortlist_t
Room::Private::buildShortlist(const QStringList& userIds) const
{
    // To calculate room display name the spec requires to sort users
    // lexicographically by state_key (user id) and use disambiguated
    // display names of two topmost users excluding the current one to render
    // the name of the room. The below code selects 3 topmost users,
    // slightly extending the spec.
    users_shortlist_t shortlist {}; // Prefill with nullptrs
    std::partial_sort_copy(
        userIds.begin(), userIds.end(), shortlist.begin(), shortlist.end(),
        [this](const QString& u1, const QString& u2) {
            // localUser(), if it's in the list, is sorted
            // below all others
            return isLocalMember(u2) || (!isLocalMember(u1) && u1 < u2);
        });
    return shortlist;
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
        memberNameMap.isEmpty()
        || (memberNameMap.size() == 1 && isLocalMember(*memberNameMap.cbegin()));
    const bool nonEmptySummary = summary.heroes && !summary.heroes->empty();
    auto shortlist = nonEmptySummary ? buildShortlist(*summary.heroes)
                                     : !emptyRoom ? buildShortlist(memberNameMap.values())
                                                  : users_shortlist_t {};

    // When the heroes list is there, we can rely on it. If the heroes list is
    // missing, the below code gathers invited, or, if there are no invitees,
    // left members.
    if (shortlist.front().isEmpty() && localUserIsIn)
        shortlist = buildShortlist(membersInvited);

    if (shortlist.front().isEmpty())
        shortlist = buildShortlist(membersLeft);

    QStringList names;
    for (const auto& u : shortlist) {
        if (u.isEmpty() || isLocalMember(u))
            break;
        // Only disambiguate if the room is not empty
        names.push_back(q->member(u).displayName());
    }

    const auto usersCountExceptLocal =
        !emptyRoom
            ? q->joinedCount() - int(joinState == JoinState::Join)
            : !membersInvited.empty()
                  ? membersInvited.count()
                  : membersLeft.size() - int(joinState == JoinState::Leave);
    if (usersCountExceptLocal > int(shortlist.size()))
        names << tr(
            "%Ln other(s)",
            "Used to make a room name from user names: A, B and _N others_",
            static_cast<int>(usersCountExceptLocal - std::ssize(shortlist)));
    const auto namesList = QLocale().createSeparatedList(names);

    // Room members
    if (!emptyRoom)
        return namesList;

    // (Spec extension) Invited users
    if (!membersInvited.empty())
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
            json[UnsignedKey] = unsignedJson;
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

MemberSorter Room::memberSorter() const { return MemberSorter(); }

void Room::activateEncryption()
{
    if(usesEncryption()) {
        qCWarning(E2EE) << "Room" << objectName() << "is already encrypted";
        return;
    }
    setState<EncryptionEvent>(EncryptionType::MegolmV1AesSha2);
}

void Room::addMegolmSessionFromBackup(const QByteArray& sessionId, const QByteArray& sessionKey, uint32_t index, const QByteArray& senderKey, const QByteArray& senderEdKey)
{
    const auto sessionIt = d->groupSessions.find(sessionId);
    if (sessionIt != d->groupSessions.end() && sessionIt->second.firstKnownIndex() <= index)
        return;

    auto&& importResult = QOlmInboundGroupSession::importSession(sessionKey);
    if (!importResult)
        return;
    // NB: after the next line, sessionIt can be invalid.
    auto& session = d->groupSessions
                        .insert_or_assign(sessionIt, sessionId,
                                          std::move(importResult.value()))
                        ->second;
    session.setOlmSessionId(d->connection->isVerifiedSession(sessionId)
                                ? QByteArrayLiteral("BACKUP_VERIFIED")
                                : QByteArrayLiteral("BACKUP"));
    session.setSenderId("BACKUP"_ls);
    d->connection->saveMegolmSession(this, session, senderKey, senderEdKey);
}

void Room::startVerification()
{
    if (joinedMembers().count() != 2) {
        return;
    }
    d->pendingKeyVerificationSession = new KeyVerificationSession(this);
    emit d->connection->newKeyVerificationSession(d->pendingKeyVerificationSession);
}

QJsonArray Room::exportMegolmSessions()
{
    QJsonArray sessions;
    for (auto& [key, value] : d->groupSessions) {
        auto session = value.exportSession(value.firstKnownIndex());
        if (!session.has_value()) {
            qCWarning(E2EE) << "Failed to export session" << session.error();
            continue;
        }

        const auto senderClaimedKey = connection()->database()->edKeyForMegolmSession(QString::fromLatin1(value.sessionId()));
        const auto senderKey = connection()->database()->senderKeyForMegolmSession(QString::fromLatin1(value.sessionId()));
        const auto json = QJsonObject {
            {"algorithm"_ls, "m.megolm.v1.aes-sha2"_ls},
            {"forwarding_curve25519_key_chain"_ls, QJsonArray()},
            {"room_id"_ls, id()},
            {"sender_claimed_keys"_ls, QJsonObject{ {"ed25519"_ls, senderClaimedKey} }},
            {"sender_key"_ls, senderKey},
            {"session_id"_ls, QString::fromLatin1(value.sessionId())},
            {"session_key"_ls, QString::fromLatin1(session.value())},
        };
        if (senderClaimedKey.isEmpty() || senderKey.isEmpty()) {
            // These are edge-cases for some sessions that were added before libquotient started storing these fields.
            // Some clients refuse to the entire file if this is missing for one key, so we shouldn't export the session in this case.
            qCWarning(E2EE) << "Session" << value.sessionId() << "has unknown sender key.";
            continue;
        }
        sessions.append(json);
    }
    return sessions;
}


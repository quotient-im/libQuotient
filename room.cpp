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
#include "jobs/setroomstatejob.h"
#include "events/simplestateevents.h"
#include "events/roomavatarevent.h"
#include "events/roommemberevent.h"
#include "events/typingevent.h"
#include "events/receiptevent.h"
#include "events/redactionevent.h"
#include "jobs/sendeventjob.h"
#include "jobs/roommessagesjob.h"
#include "avatar.h"
#include "connection.h"
#include "user.h"

#include <QtCore/QHash>
#include <QtCore/QStringBuilder> // for efficient string concats (operator%)
#include <QtCore/QElapsedTimer>

#include <array>
#include <functional>

using namespace QMatrixClient;
using namespace std::placeholders;

enum EventsPlacement : int { Older = -1, Newer = 1 };

class Room::Private
{
    public:
        /** Map of user names to users. User names potentially duplicate, hence a multi-hashmap. */
        typedef QMultiHash<QString, User*> members_map_t;
        typedef std::pair<rev_iter_t, rev_iter_t> rev_iter_pair_t;

        Private(Connection* c, QString id_, JoinState initialJoinState)
            : q(nullptr), connection(c), id(std::move(id_))
            , avatar(c), joinState(initialJoinState), unreadMessages(false)
            , highlightCount(0), notificationCount(0), roomMessagesJob(nullptr)
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
        Avatar avatar;
        JoinState joinState;
        bool unreadMessages;
        int highlightCount;
        int notificationCount;
        members_map_t membersMap;
        QList<User*> usersTyping;
        QList<User*> membersLeft;
        QHash<const User*, QString> lastReadEventIds;
        QString prevBatch;
        RoomMessagesJob* roomMessagesJob;

        // Convenience methods to work with the membersMap and usersLeft.
        // addMember() and removeMember() emit respective Room:: signals
        // after a succesful operation.
        //void inviteUser(User* u); // We might get it at some point in time.
        void addMember(User* u);
        bool hasMember(User* u) const;
        // You can't identify a single user by displayname, only by id
        User* member(const QString& id) const;
        void renameMember(User* u, QString oldName);
        void removeMember(User* u);

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
        void checkUnreadMessages(timeline_iter_t from);

        void setLastReadEvent(User* u, const QString& eventId);
        rev_iter_pair_t promoteReadMarker(User* u, rev_iter_t newMarker,
                                          bool force = false);

        void markMessagesAsRead(rev_iter_t upToMarker);

        /**
         * @brief Apply redaction to the timeline
         *
         * Tries to find an event in the timeline and redact it; deletes the
         * redaction event whether the redacted event was found or not.
         */
        void processRedaction(RoomEventPtr redactionEvent);

        QJsonObject toJson() const;

    private:
        QString calculateDisplayname() const;
        QString roomNameFromMemberNames(const QList<User*>& userlist) const;

        void insertMemberIntoMap(User* u);
        void removeMemberFromMap(const QString& username, User* u);

        bool isLocalUser(const User* u) const
        {
            return u == connection->user();
        }
};

RoomEventPtr TimelineItem::replaceEvent(RoomEventPtr&& other)
{
    return std::exchange(evt, std::move(other));
}

Room::Room(Connection* connection, QString id, JoinState initialJoinState)
    : QObject(connection), d(new Private(connection, id, initialJoinState))
{
    // See "Accessing the Public Class" section in
    // https://marcmutz.wordpress.com/translated-articles/pimp-my-pimpl-%E2%80%94-reloaded/
    d->q = this;
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

QImage Room::avatar(int dimension)
{
    return avatar(dimension, dimension);
}

QImage Room::avatar(int width, int height)
{
    if (!d->avatar.url().isEmpty())
        return d->avatar.get(width, height, [=] { emit avatarChanged(); });

    // Use the other side's avatar for 1:1's
    if (d->membersMap.size() == 2)
    {
        auto theOtherOneIt = d->membersMap.begin();
        if (theOtherOneIt.value() == localUser())
            ++theOtherOneIt;
        return theOtherOneIt.value()->avatarObject()
                .get(width, height, [=] { emit avatarChanged(); });
    }
    return {};
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
    lastReadEventIds.insert(u, eventId);
    emit q->lastReadEventChanged(u);
    if (isLocalUser(u))
        emit q->readMarkerMoved();
}

Room::Private::rev_iter_pair_t
Room::Private::promoteReadMarker(User* u, Room::rev_iter_t newMarker,
                                 bool force)
{
    Q_ASSERT_X(u, __FUNCTION__, "User* should not be nullptr");
    Q_ASSERT(newMarker >= timeline.crbegin() && newMarker <= timeline.crend());

    const auto prevMarker = q->readMarker(u);
    if (!force && prevMarker <= newMarker) // Remember, we deal with reverse iterators
        return { prevMarker, prevMarker };

    Q_ASSERT(newMarker < timeline.crend());

    // Try to auto-promote the read marker over the user's own messages
    // (switch to direct iterators for that).
    auto eagerMarker = find_if(newMarker.base(), timeline.cend(),
          [=](const TimelineItem& ti) { return ti->senderId() != u->id(); });

    setLastReadEvent(u, (*(eagerMarker - 1))->id());
    if (isLocalUser(u) && unreadMessages)
    {
        auto stillUnreadMessagesCount = count_if(eagerMarker, timeline.cend(),
                std::bind(&Room::Private::isEventNotable, this, _1));

        if (stillUnreadMessagesCount == 0)
        {
            unreadMessages = false;
            qCDebug(MAIN) << "Room" << displayname << "has no more unread messages";
            emit q->unreadMessagesChanged(q);
        } else
            qCDebug(MAIN) << "Room" << displayname << "still has"
                          << stillUnreadMessagesCount << "unread message(s)";
    }

    // Return newMarker, rather than eagerMarker, to save markMessagesAsRead()
    // (that calls this method) from going back through knowingly-local messages.
    return { prevMarker, newMarker };
}

void Room::Private::markMessagesAsRead(Room::rev_iter_t upToMarker)
{
    rev_iter_pair_t markers = promoteReadMarker(q->localUser(), upToMarker);
    if (markers.first != markers.second)
        qCDebug(MAIN) << "Marked messages as read until" << *q->readMarker();

    // We shouldn't send read receipts for the local user's own messages - so
    // search earlier messages for the latest message not from the local user
    // until the previous last-read message, whichever comes first.
    for (; markers.second < markers.first; ++markers.second)
    {
        if ((*markers.second)->senderId() != q->localUser()->id())
        {
            connection->callApi<PostReceiptJob>(
                id, "m.read", (*markers.second)->id());
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

bool Room::hasUnreadMessages()
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
        res.append( this->roomMembername(u) );

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

void Room::Private::insertMemberIntoMap(User *u)
{
    auto namesakes = membersMap.values(u->name());
    membersMap.insert(u->name(), u);
    // If there is exactly one namesake of the added user, signal member renaming
    // for that other one because the two should be disambiguated now.
    if (namesakes.size() == 1)
        emit q->memberRenamed(namesakes[0]);
}

void Room::Private::removeMemberFromMap(const QString& username, User* u)
{
    membersMap.remove(username, u);
    // If there was one namesake besides the removed user, signal member renaming
    // for it because it doesn't need to be disambiguated anymore.
    // TODO: Think about left users.
    auto formerNamesakes = membersMap.values(username);
    if (formerNamesakes.size() == 1)
        emit q->memberRenamed(formerNamesakes[0]);
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

void Room::Private::addMember(User *u)
{
    if (!hasMember(u))
    {
        insertMemberIntoMap(u);
        connect(u, &User::nameChanged, q,
                bind(&Private::renameMember, this, _1, _2));
        emit q->userAdded(u);
    }
}

bool Room::Private::hasMember(User* u) const
{
    return membersMap.values(u->name()).contains(u);
}

User* Room::Private::member(const QString& id) const
{
    auto u = connection->user(id);
    return hasMember(u) ? u : nullptr;
}

void Room::Private::renameMember(User* u, QString oldName)
{
    if (hasMember(u))
    {
        qCWarning(MAIN) << "Room::Private::renameMember(): the user "
                        << u->name()
                        << "is already known in the room under a new name.";
        return;
    }

    if (membersMap.values(oldName).contains(u))
    {
        removeMemberFromMap(oldName, u);
        insertMemberIntoMap(u);
        emit q->memberRenamed(u);
    }
}

void Room::Private::removeMember(User* u)
{
    if (hasMember(u))
    {
        if ( !membersLeft.contains(u) )
            membersLeft.append(u);
        removeMemberFromMap(u->name(), u);
        emit q->userRemoved(u);
    }
}

QString Room::roomMembername(User *u) const
{
    // See the CS spec, section 11.2.2.3

    QString username = u->name();
    if (username.isEmpty())
        return u->id();

    // Get the list of users with the same display name. Most likely,
    // there'll be one, but there's a chance there are more.
    auto namesakes = d->membersMap.values(username);
    if (namesakes.size() == 1)
        return username;

    // We expect a user to be a member of the room - but technically it is
    // possible to invoke roomMemberName() even for non-members. In such case
    // we return the name _with_ id, to stay on a safe side.
    // XXX: Causes a storm of false alarms when scrolling through older events
    // with left users; commented out until we have a proper backtracking of
    // room state ("room time machine").
//    if ( !namesakes.contains(u) )
//    {
//        qCWarning()
//            << "Room::roomMemberName(): user" << u->id()
//            << "is not a member of the room" << id();
//    }

    // In case of more than one namesake, disambiguate with user id.
    return username % " (" % u->id() % ")";
}

QString Room::roomMembername(const QString& userId) const
{
    return roomMembername(connection()->user(userId));
}

void Room::updateData(SyncRoomData&& data)
{
    if( d->prevBatch.isEmpty() )
        d->prevBatch = data.timelinePrevBatch;
    setJoinState(data.joinState);

    QElapsedTimer et;
    if (!data.state.empty())
    {
        et.start();
        processStateEvents(data.state);
        qCDebug(PROFILER) << "*** Room::processStateEvents(state):"
            << et.elapsed() << "ms," << data.state.size() << "events";
    }
    if (!data.timeline.empty())
    {
        et.restart();
        // State changes can arrive in a timeline event; so check those.
        processStateEvents(data.timeline);
        qCDebug(PROFILER) << "*** Room::processStateEvents(timeline):"
            << et.elapsed() << "ms," << data.timeline.size() << "events";

        et.restart();
        d->addNewMessageEvents(move(data.timeline));
        qCDebug(PROFILER) << "*** Room::addNewMessageEvents():"
                          << et.elapsed() << "ms";
    }
    if (!data.ephemeral.empty())
    {
        et.restart();
        for( auto&& ephemeralEvent: data.ephemeral )
            processEphemeralEvent(move(ephemeralEvent));
        qCDebug(PROFILER) << "*** Room::processEphemeralEvents():"
                          << et.elapsed() << "ms";
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
    connection()->callApi<SendEventJob>(id(), event);
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
    if( !roomMessagesJob )
    {
        roomMessagesJob =
                connection->callApi<RoomMessagesJob>(id, prevBatch, limit);
        connect( roomMessagesJob, &RoomMessagesJob::result, [=] {
            if( !roomMessagesJob->error() )
            {
                addHistoricalMessageEvents(roomMessagesJob->releaseEvents());
                prevBatch = roomMessagesJob->end();
            }
            roomMessagesJob = nullptr;
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

void Room::Private::processRedaction(RoomEventPtr redactionEvent)
{
    Q_ASSERT(redactionEvent && isRedaction(redactionEvent));
    const auto& redaction =
            static_cast<const RedactionEvent*>(redactionEvent.get());

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
    q->onRedaction(oldEvent.get(), ti.event());
    qCDebug(MAIN) << "Redacted" << oldEvent->id() << "with" << redaction->id();
    emit q->replacedEvent(ti.event(), oldEvent.get());
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
    const auto insertedSize = insertEvents(std::move(normalEvents), Newer);
    if (insertedSize > 0)
    {
        qCDebug(MAIN)
                << "Room" << displayname << "received" << insertedSize
                << "new events; the last event is now" << timeline.back();
        q->onAddNewTimelineEvents(timeline.cend() - insertedSize);
    }
    for (auto&& r: redactions)
        processRedaction(move(r));
    if (insertedSize > 0)
    {
        checkUnreadMessages(timeline.cend() - insertedSize);
        emit q->addedMessages();
    }

    Q_ASSERT(timeline.size() == timelineSize + insertedSize);
}

void Room::Private::checkUnreadMessages(timeline_iter_t from)
{
    Q_ASSERT(from < timeline.cend());
    const auto newUnreadMessages = count_if(from, timeline.cend(),
            std::bind(&Room::Private::isEventNotable, this, _1));

    // The first event in the just-added batch (referred to by upTo.base())
    // defines whose read marker can possibly be promoted any further over
    // the same author's events newly arrived. Others will need explicit
    // read receipts from the server (or, for the local user,
    // markMessagesAsRead() invocation) to promote their read markers over
    // the new message events.
    auto firstWriter = connection->user((*from)->senderId());
    if (q->readMarker(firstWriter) != timeline.crend())
    {
        promoteReadMarker(firstWriter, q->findInTimeline((*from)->id()));
        qCDebug(MAIN) << "Auto-promoted read marker for" << firstWriter->id()
                      << "to" << *q->readMarker(firstWriter);
    }

    if(!unreadMessages && newUnreadMessages > 0)
    {
        unreadMessages = true;
        emit q->unreadMessagesChanged(q);
        qCDebug(MAIN) << "Room" << displayname << "has unread messages";
    }
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
    const bool thereWasNoReadMarker = q->readMarker() == timeline.crend();
    const auto insertedSize = insertEvents(std::move(normalEvents), Older);

    // Catch a special case when the last read event id refers to an event
    // that was outside the loaded timeline and has just arrived. Depending on
    // other messages next to the last read one, we might need to promote
    // the read marker and update unreadMessages flag.
    const auto curReadMarker = q->readMarker();
    if (thereWasNoReadMarker && curReadMarker != timeline.crend())
    {
        qCDebug(MAIN) << "Discovered last read event in a historical batch";
        promoteReadMarker(q->localUser(), curReadMarker, true);
    }
    qCDebug(MAIN) << "Room" << displayname << "received" << insertedSize
                  << "past events; the oldest event is now" << timeline.front();
    q->onAddHistoricalTimelineEvents(timeline.crend() - insertedSize);
    emit q->addedMessages();

    Q_ASSERT(timeline.size() == timelineSize + insertedSize);
}

void Room::processStateEvents(const RoomEvents& events)
{
    bool emitNamesChanged = false;
    for (const auto& e: events)
    {
        auto* event = e.get();
        switch (event->type())
        {
            case EventType::RoomName: {
                auto nameEvent = static_cast<RoomNameEvent*>(event);
                d->name = nameEvent->name();
                qCDebug(MAIN) << "Room name updated:" << d->name;
                emitNamesChanged = true;
                break;
            }
            case EventType::RoomAliases: {
                auto aliasesEvent = static_cast<RoomAliasesEvent*>(event);
                d->aliases = aliasesEvent->aliases();
                qCDebug(MAIN) << "Room aliases updated:" << d->aliases;
                emitNamesChanged = true;
                break;
            }
            case EventType::RoomCanonicalAlias: {
                auto aliasEvent = static_cast<RoomCanonicalAliasEvent*>(event);
                d->canonicalAlias = aliasEvent->alias();
                qCDebug(MAIN) << "Room canonical alias updated:" << d->canonicalAlias;
                emitNamesChanged = true;
                break;
            }
            case EventType::RoomTopic: {
                auto topicEvent = static_cast<RoomTopicEvent*>(event);
                d->topic = topicEvent->topic();
                qCDebug(MAIN) << "Room topic updated:" << d->topic;
                emit topicChanged();
                break;
            }
            case EventType::RoomAvatar: {
                const auto& avatarEventContent =
                        static_cast<RoomAvatarEvent*>(event)->content();
                if (d->avatar.updateUrl(avatarEventContent.url))
                {
                    qCDebug(MAIN) << "Room avatar URL updated:"
                                  << avatarEventContent.url.toString();
                    emit avatarChanged();
                }
                break;
            }
            case EventType::RoomMember: {
                auto memberEvent = static_cast<RoomMemberEvent*>(event);
                // Can't use d->member() below because the user may be not a member (yet)
                auto u = d->connection->user(memberEvent->userId());
                u->processEvent(event);
                if( memberEvent->membership() == MembershipType::Join )
                {
                    d->addMember(u);
                }
                else if( memberEvent->membership() == MembershipType::Leave )
                {
                    d->removeMember(u);
                }
                break;
            }
            default: /* Ignore events of other types */;
        }
    }
    if (emitNamesChanged) {
        emit namesChanged(this);
    }
    d->updateDisplayname();
}

void Room::processEphemeralEvent(EventPtr event)
{
    switch (event->type())
    {
        case EventType::Typing: {
            auto typingEvent = static_cast<TypingEvent*>(event.get());
            d->usersTyping.clear();
            for( const QString& userId: typingEvent->users() )
            {
                if (auto m = d->member(userId))
                    d->usersTyping.append(m);
            }
            emit typingChanged();
            break;
        }
        case EventType::Receipt: {
            auto receiptEvent = static_cast<ReceiptEvent*>(event.get());
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
                if (d->eventsIndex.contains(p.evtId))
                {
                    const auto newMarker = findInTimeline(p.evtId);
                    for( const Receipt& r: p.receipts )
                        if (auto m = d->member(r.userId))
                            d->promoteReadMarker(m, newMarker);
                } else
                {
                    qCDebug(EPHEMERAL) << "Event" << p.evtId
                                       << "not found; saving read receipts anyway";
                    // If the event is not found (most likely, because it's too old
                    // and hasn't been fetched from the server yet), but there is
                    // a previous marker for a user, keep the previous marker.
                    // Otherwise, blindly store the event id for this user.
                    for( const Receipt& r: p.receipts )
                        if (auto m = d->member(r.userId))
                            if (readMarker(m) == timelineEdge())
                                d->setLastReadEvent(m, p.evtId);
                }
            }
            if (receiptEvent->unreadMessages())
                d->unreadMessages = true;
            break;
        }
        default:
            qCWarning(EPHEMERAL) << "Unexpected event type in 'ephemeral' batch:"
                                 << event->type();
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

template <typename T>
void appendStateEvent(QJsonArray& events, const QString& type,
                     const QString& name, const T& content)
{
    if (content.isEmpty())
        return;

    QJsonObject contentObj;
    contentObj.insert(name, content);

    QJsonObject eventObj;
    eventObj.insert("type", type);
    eventObj.insert("content", contentObj);
    eventObj.insert("state_key", {}); // Mandatory for state events

    events.append(eventObj);
}

QJsonObject Room::Private::toJson() const
{
    QJsonObject result;
    {
        QJsonArray stateEvents;

        appendStateEvent(stateEvents, "m.room.name", "name", name);
        appendStateEvent(stateEvents, "m.room.topic", "topic", topic);
        appendStateEvent(stateEvents, "m.room.avatar", "url",
                         avatar.url().toString());
        appendStateEvent(stateEvents, "m.room.aliases", "aliases",
                         QJsonArray::fromStringList(aliases));
        appendStateEvent(stateEvents, "m.room.canonical_alias", "alias",
                         canonicalAlias);

        for (const auto &i : membersMap)
        {
            QJsonObject content;
            content.insert("membership", QStringLiteral("join"));
            content.insert("displayname", i->name());
            content.insert("avatar_url", i->avatarUrl().toString());

            QJsonObject memberEvent;
            memberEvent.insert("type", QStringLiteral("m.room.member"));
            memberEvent.insert("state_key", i->id());
            memberEvent.insert("content", content);
            stateEvents.append(memberEvent);
        }

        QJsonObject roomStateObj;
        roomStateObj.insert("events", stateEvents);

        result.insert("state", roomStateObj);
    }

    if (!q->readMarkerEventId().isEmpty())
    {
        QJsonArray ephemeralEvents;
        {
            // Don't dump the timestamp because it's useless in the cache.
            QJsonObject user;
            user.insert(connection->userId(), {});

            QJsonObject receipt;
            receipt.insert("m.read", user);

            QJsonObject lastReadEvent;
            lastReadEvent.insert(q->readMarkerEventId(), receipt);
            lastReadEvent.insert("x-qmatrixclient.unread_messages",
                                 unreadMessages);

            QJsonObject receiptsObj;
            receiptsObj.insert("type", QStringLiteral("m.receipt"));
            receiptsObj.insert("content", lastReadEvent);
            ephemeralEvents.append(receiptsObj);
        }

        QJsonObject ephemeralObj;
        ephemeralObj.insert("events", ephemeralEvents);

        result.insert("ephemeral", ephemeralObj);
    }

    QJsonObject unreadNotificationsObj;
    unreadNotificationsObj.insert("highlight_count", highlightCount);
    unreadNotificationsObj.insert("notification_count", notificationCount);
    result.insert("unread_notifications", unreadNotificationsObj);

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
    auto n1 = room->roomMembername(u1);
    auto n2 = room->roomMembername(u2);
    if (n1.startsWith('@'))
        n1.remove(0, 1);
    if (n2.startsWith('@'))
        n2.remove(0, 1);
    return n1.localeAwareCompare(n2) < 0;
}

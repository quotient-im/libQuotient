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
#include "jobs/setroomstatejob.h"
#include "events/roomnameevent.h"
#include "events/roomaliasesevent.h"
#include "events/roomcanonicalaliasevent.h"
#include "events/roomtopicevent.h"
#include "events/roomavatarevent.h"
#include "events/roommemberevent.h"
#include "events/typingevent.h"
#include "events/receiptevent.h"
#include "jobs/sendeventjob.h"
#include "jobs/roommessagesjob.h"
#include "jobs/postreceiptjob.h"
#include "avatar.h"
#include "connection.h"
#include "user.h"

#include <QtCore/QHash>
#include <QtCore/QStringBuilder> // for efficient string concats (operator%)
#include <QtCore/QElapsedTimer>

#include <array>

using namespace QMatrixClient;

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

        bool isEventNotable(const RoomEvent* e) const
        {
            return e->senderId() != connection->userId() &&
                    e->type() == EventType::RoomMessage;
        }

        void appendEvent(RoomEvent* e)
        {
            insertEvent(e, timeline.end(),
                        timeline.empty() ? 0 : q->maxTimelineIndex() + 1);
        }
        void prependEvent(RoomEvent* e)
        {
            insertEvent(e, timeline.begin(),
                        timeline.empty() ? 0 : q->minTimelineIndex() - 1);
        }

        /**
         * Removes events from the passed container that are already in the timeline
         */
        void dropDuplicateEvents(RoomEvents* events) const;

        void setLastReadEvent(User* u, const QString& eventId);
        rev_iter_pair_t promoteReadMarker(User* u, rev_iter_t newMarker,
                                          bool force = false);

        QJsonObject toJson() const;

    private:
        QString calculateDisplayname() const;
        QString roomNameFromMemberNames(const QList<User*>& userlist) const;

        void insertMemberIntoMap(User* u);
        void removeMemberFromMap(const QString& username, User* u);

        void insertEvent(RoomEvent* e, Timeline::iterator where,
                         TimelineItem::index_t index);
        bool isLocalUser(const User* u) const
        {
            return u == connection->user();
        }
};

Room::Room(Connection* connection, QString id, JoinState initialJoinState)
    : QObject(connection), d(new Private(connection, id, initialJoinState))
{
    // See "Accessing the Public Class" section in
    // https://marcmutz.wordpress.com/translated-articles/pimp-my-pimpl-%E2%80%94-reloaded/
    d->q = this;
    qCDebug(MAIN) << "New Room:" << id;
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

QPixmap Room::avatar(int width, int height)
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
        auto stillUnreadMessagesCount =
            count_if(eagerMarker, timeline.cend(),
                 [=](const TimelineItem& ti) { return isEventNotable(ti.event()); });

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

void Room::markMessagesAsRead(Room::rev_iter_t upToMarker)
{
    Private::rev_iter_pair_t markers = d->promoteReadMarker(localUser(), upToMarker);
    if (markers.first != markers.second)
        qCDebug(MAIN) << "Marked messages as read until" << *readMarker();

    // We shouldn't send read receipts for the local user's own messages - so
    // search earlier messages for the latest message not from the local user
    // until the previous last-read message, whichever comes first.
    for (; markers.second < markers.first; ++markers.second)
    {
        if ((*markers.second)->senderId() != localUser()->id())
        {
            connection()->callApi<PostReceiptJob>(id(), (*markers.second)->id());
            break;
        }
    }
}

void Room::markMessagesAsRead(QString uptoEventId)
{
    markMessagesAsRead(findInTimeline(uptoEventId));
}

void Room::markAllMessagesAsRead()
{
    if (!d->timeline.empty())
        markMessagesAsRead(d->timeline.crbegin());
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

inline QByteArray makeErrorStr(const Event* e, QByteArray msg)
{
    return msg.append("; event dump follows:\n").append(e->originalJson());
}

void Room::Private::insertEvent(RoomEvent* e, Timeline::iterator where,
                                TimelineItem::index_t index)
{
    Q_ASSERT_X(e, __FUNCTION__, "Attempt to add nullptr to timeline");
    Q_ASSERT_X(!e->id().isEmpty(), __FUNCTION__,
               makeErrorStr(e, "Event with empty id cannot be in the timeline"));
    Q_ASSERT_X(where == timeline.end() || where == timeline.begin(), __FUNCTION__,
               "Events can only be appended or prepended to the timeline");
    if (eventsIndex.contains(e->id()))
    {
        qCWarning(MAIN) << "Event" << e->id() << "is already in the timeline.";
        qCWarning(MAIN) << "Either dropDuplicateEvents() wasn't called or duplicate "
                           "events within the same batch arrived from the server.";
        return;
    }
    timeline.emplace(where, e, index);
    eventsIndex.insert(e->id(), index);
    Q_ASSERT(q->findInTimeline(e->id())->event() == e);
}

void Room::Private::addMember(User *u)
{
    if (!hasMember(u))
    {
        insertMemberIntoMap(u);
        connect(u, &User::nameChanged, q,
                [=] (User* u, const QString& newName) { renameMember(u, newName); });
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
        addNewMessageEvents(data.timeline.release());
        qCDebug(PROFILER) << "*** Room::addNewMessageEvents():"
                          << et.elapsed() << "ms";
    }
    if (!data.ephemeral.empty())
    {
        et.restart();
        for( auto ephemeralEvent: data.ephemeral )
            processEphemeralEvent(ephemeralEvent);
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
        connect( roomMessagesJob, &RoomMessagesJob::result, [=]() {
            if( !roomMessagesJob->error() )
            {
                q->addHistoricalMessageEvents(roomMessagesJob->releaseEvents());
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

void Room::Private::dropDuplicateEvents(RoomEvents* events) const
{
    // Collect all duplicate events at the end of the container
    auto dupsBegin =
            std::stable_partition(events->begin(), events->end(),
                  [&] (RoomEvent* e) { return !eventsIndex.contains(e->id()); });
    // Dispose of those dups
    std::for_each(dupsBegin, events->end(), [] (Event* e) { delete e; });
    events->erase(dupsBegin, events->end());
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

void Room::addNewMessageEvents(RoomEvents events)
{
    d->dropDuplicateEvents(&events);
    if (events.empty())
        return;
    emit aboutToAddNewMessages(events);
    doAddNewMessageEvents(events);
    emit addedMessages();
}

void Room::doAddNewMessageEvents(const RoomEvents& events)
{
    Q_ASSERT(!events.empty());

    Timeline::size_type newUnreadMessages = 0;
    for (auto e: events)
    {
        d->appendEvent(e);
        newUnreadMessages += d->isEventNotable(e);
    }
    qCDebug(MAIN) << "Room" << displayName() << "received" << events.size()
                  << "(with" << newUnreadMessages << "notable)"
                  << "new events; the last event is now" << d->timeline.back();

    // The first event in the batch defines whose read marker can possibly be
    // promoted any further over the same author's events newly arrived.
    // Others will need explicit read receipts from the server (or, for
    // the local user, markMessagesAsRead() invocation) to promote their
    // read markers over the new message events.
    User* firstWriter = connection()->user(events.front()->senderId());
    if (readMarker(firstWriter) != timelineEdge())
    {
        d->promoteReadMarker(firstWriter, findInTimeline(events.front()->id()));
        qCDebug(MAIN) << "Auto-promoted read marker for" << firstWriter->id()
                      << "to" << *readMarker(firstWriter);
    }

    if( !d->unreadMessages && newUnreadMessages > 0)
    {
        d->unreadMessages = true;
        emit unreadMessagesChanged(this);
        qCDebug(MAIN) << "Room" << displayName() << "has unread messages";
    }
}

void Room::addHistoricalMessageEvents(RoomEvents events)
{
    d->dropDuplicateEvents(&events);
    if (events.empty())
        return;
    emit aboutToAddHistoricalMessages(events);
    doAddHistoricalMessageEvents(events);
    emit addedMessages();
}

void Room::doAddHistoricalMessageEvents(const RoomEvents& events)
{
    Q_ASSERT(!events.empty());

    const bool thereWasNoReadMarker = readMarker() == timelineEdge();
    // Historical messages arrive in newest-to-oldest order
    for (auto e: events)
        d->prependEvent(e);

    // Catch a special case when the last read event id refers to an event
    // that was outside the loaded timeline and has just arrived. Depending on
    // other messages next to the last read one, we might need to promote
    // the read marker and update unreadMessages flag.
    const auto curReadMarker = readMarker();
    if (thereWasNoReadMarker && curReadMarker != timelineEdge())
    {
        qCDebug(MAIN) << "Discovered last read event in a historical batch";
        d->promoteReadMarker(localUser(), curReadMarker, true);
    }
    qCDebug(MAIN) << "Room" << displayName() << "received" << events.size()
                  << "past events; the oldest event is now" << d->timeline.front();
}

void Room::processStateEvents(const RoomEvents& events)
{
    bool emitNamesChanged = false;
    for (auto event: events)
    {
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

void Room::processEphemeralEvent(Event* event)
{
    switch (event->type())
    {
        case EventType::Typing: {
            auto typingEvent = static_cast<TypingEvent*>(event);
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
            auto receiptEvent = static_cast<ReceiptEvent*>(event);
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
void appendEventJson(QJsonArray& events, const QString& type,
                     const QString& name, const T& content)
{
    if (content.isEmpty())
        return;

    QJsonObject contentObj;
    contentObj.insert(name, content);

    QJsonObject eventObj;
    eventObj.insert("type", type);
    eventObj.insert("content", contentObj);

    events.append(eventObj);
}

QJsonObject Room::Private::toJson() const
{
    QJsonObject result;
    {
        QJsonArray stateEvents;

        appendEventJson(stateEvents, "m.room.name", "name", name);
        appendEventJson(stateEvents, "m.room.topic", "topic", topic);
        appendEventJson(stateEvents, "m.room.avatar", "avatar_url",
                        avatar.url().toString());
        appendEventJson(stateEvents, "m.room.aliases", "aliases",
                        QJsonArray::fromStringList(aliases));
        appendEventJson(stateEvents, "m.room.canonical_alias", "alias",
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
    if (n1[0] == '@')
        n1.remove(0, 1);
    if (n2[0] == '@')
        n2.remove(0, 1);
    return n1.localeAwareCompare(n2) < 0;
}

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

#include <array>

#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QStringBuilder> // for efficient string concats (operator%)
#include <QtCore/QDebug>

#include "connection.h"
#include "state.h"
#include "user.h"
#include "events/roommessageevent.h"
#include "events/roomnameevent.h"
#include "events/roomaliasesevent.h"
#include "events/roomcanonicalaliasevent.h"
#include "events/roomtopicevent.h"
#include "events/roommemberevent.h"
#include "events/typingevent.h"
#include "events/receiptevent.h"
#include "jobs/postmessagejob.h"
#include "jobs/roommessagesjob.h"
#include "jobs/postreceiptjob.h"

using namespace QMatrixClient;

class Room::Private
{
    public:
        /** Map of user names to users. User names potentially duplicate, hence a multi-hashmap. */
        typedef QMultiHash<QString, User*> members_map_t;
        typedef Timeline::const_reverse_iterator rev_iter_t;

        Private(Connection* c, const QString& id_)
            : q(nullptr), connection(c), id(id_), joinState(JoinState::Join)
            , unreadMessages(false), highlightCount(0), notificationCount(0)
            , roomMessagesJob(nullptr)
        { }

        Room* q;

        // This updates the room displayname field (which is the way a room
        // should be shown in the room list) It should be called whenever the
        // list of members or the room name (m.room.name) or canonical alias change.
        void updateDisplayname();

        Connection* connection;
        Timeline timeline;
        QString id;
        QStringList aliases;
        QString canonicalAlias;
        QString name;
        QString displayname;
        QString topic;
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
        User* member(QString id) const;
        void renameMember(User* u, QString oldName);
        void removeMember(User* u);

        void getPreviousContent(int limit = 10);

        bool isEventNotable(const Event* e) const;

        void appendEvent(Event* e)
        {
            insertEvent(e, timeline.end());
        }
        void prependEvent(Event* e)
        {
            insertEvent(e, timeline.begin());
        }

        /**
         * Removes events from the passed container that are already in the timeline
         */
        void dropDuplicateEvents(Events& events) const
        {
            // Collect all duplicate events at the end of the container
            auto dupsBegin =
                std::stable_partition(events.begin(), events.end(),
                    [&] (Event* e) { return !eventsIndex.contains(e->id()); });
            // Dispose of those dups
            std::for_each(dupsBegin, events.end(), [] (Event* e) { delete e; });
            events.erase(dupsBegin, events.end());
        }

        rev_iter_t readMarker(const User* user) const
        {
            return eventsIndex.value(lastReadEventIds[user], timeline.crend());
        }
        std::pair<rev_iter_t, rev_iter_t> promoteReadMarker(User* u, QString eventId);

    private:
        QHash<QString, rev_iter_t> eventsIndex;

        QString calculateDisplayname() const;
        QString roomNameFromMemberNames(const QList<User*>& userlist) const;

        void insertMemberIntoMap(User* u);
        void removeMemberFromMap(QString username, User* u);

        void insertEvent(Event* e, Timeline::iterator where);
};

Room::Room(Connection* connection, QString id)
    : QObject(connection), d(new Private(connection, id))
{
    // See "Accessing the Public Class" section in
    // https://marcmutz.wordpress.com/translated-articles/pimp-my-pimpl-%E2%80%94-reloaded/
    d->q = this;
    qDebug() << "New Room:" << id;

    //connection->getMembers(this); // I don't think we need this anymore in r0.0.1
}

Room::~Room()
{
    delete d;
}

QString Room::id() const
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
    emit joinStateChanged(oldState, state);
}

void Room::setLastReadEvent(User* user, Event* event)
{
    d->lastReadEventIds.insert(user, event->id());
    emit lastReadEventChanged(user);
    if (user == d->connection->user())
        emit readMarkerPromoted();
}

std::pair<Room::Private::rev_iter_t, Room::Private::rev_iter_t>
Room::Private::promoteReadMarker(User* u, QString eventId)
{
    Q_ASSERT_X (!eventId.isEmpty(), __FUNCTION__,
        "Attempt to promote a read marker to an event with no id");

    // Find the event by its id and position the marker at it. If the event
    // is not found (most likely, because it's not fetched from the server),
    // or if it's older than the current marker position, keep the marker intact.
    const auto newMarker = eventsIndex.value(eventId, timeline.crend());
    const auto prevMarker = readMarker(u);
    if (prevMarker <= newMarker) // Remember, we deal with reverse iterators
        return { prevMarker, prevMarker };

    using namespace std;

    // Try to auto-promote the read marker over the user's own messages
    // (switch to direct iterators for that).
    auto eagerMarker = find_if(newMarker.base(), timeline.cend(),
                 [=](Event* e) { return e->senderId() != u->id(); });
    if (eagerMarker > timeline.begin())
        q->setLastReadEvent(u, *(eagerMarker - 1));

    if (u == connection->user() && unreadMessages)
    {
        auto stillUnreadMessagesCount =
            count_if(eagerMarker, timeline.cend(),
                     [=](Event* e) { return isEventNotable(e); });

        if (stillUnreadMessagesCount == 0)
        {
            unreadMessages = false;
            qDebug() << "Room" << q->displayName()
                     << "has no more unread messages";
            emit q->unreadMessagesChanged(q);
        }
        else
            qDebug() << "Room" << q->displayName() << "still has"
                     << stillUnreadMessagesCount << "unread message(s)";
    }

    // Return newMarker, rather than eagerMarker, to save markMessagesAsRead()
    // (that calls this method) from going back through knowingly-local messages.
    return { prevMarker, newMarker };
}

void Room::markMessagesAsRead(QString uptoEventId)
{
    auto markers = d->promoteReadMarker(connection()->user(), uptoEventId);

    // We shouldn't send read receipts for the local user's own messages - so
    // search earlier messages for the latest message not from the local user
    // until the previous last-read message, whichever comes first.
    for (; markers.second < markers.first; ++markers.second)
    {
        if ((*markers.second)->senderId() != connection()->userId())
        {
            connection()->callApi<PostReceiptJob>(this->id(),
                                                     (*markers.second)->id());
            break;
        }
    }
}

bool Room::hasUnreadMessages()
{
    return d->unreadMessages;
}

QString Room::readMarkerEventId() const
{
    return d->lastReadEventIds.value(d->connection->user());
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

void Room::Private::insertMemberIntoMap(User *u)
{
    QList<User*> namesakes = membersMap.values(u->name());
    membersMap.insert(u->name(), u);
    // If there is exactly one namesake of the added user, signal member renaming
    // for that other one because the two should be disambiguated now.
    if (namesakes.size() == 1)
        emit q->memberRenamed(namesakes[0]);

    updateDisplayname();
}

void Room::Private::removeMemberFromMap(QString username, User* u)
{
    membersMap.remove(username, u);
    // If there was one namesake besides the removed user, signal member renaming
    // for it because it doesn't need to be disambiguated anymore.
    // TODO: Think about left users.
    QList<User*> formerNamesakes = membersMap.values(username);
    if (formerNamesakes.size() == 1)
        emit q->memberRenamed(formerNamesakes[0]);

    updateDisplayname();
}

inline QByteArray makeErrorStr(Event* e, const char* msg)
{
    return QString("%1; event dump follows:\n%2")
            .arg(msg, e->originalJson()).toUtf8();
}

void Room::Private::insertEvent(Event* e, Room::Timeline::iterator where)
{
    Q_ASSERT_X(e, __FUNCTION__, "Attempt to add nullptr to timeline");
    Q_ASSERT_X(!e->id().isEmpty(), __FUNCTION__,
               makeErrorStr(e,
                            "Event with empty id cannot be in the timeline"));
    Q_ASSERT_X(!eventsIndex.contains(e->id()), __FUNCTION__,
               makeErrorStr(e,
                            "Event is already in the timeline (use dropDuplicateEvents())"));
    Q_ASSERT(where >= timeline.begin() && where <= timeline.end());
    eventsIndex.insert(e->id(), rev_iter_t(timeline.insert(where, e) + 1));
    Q_ASSERT((*eventsIndex.value(e->id()))->id() == e->id());
}

void Room::Private::addMember(User *u)
{
    if (!hasMember(u))
    {
        insertMemberIntoMap(u);
        connect(u, &User::nameChanged, q, &Room::userRenamed);
        emit q->userAdded(u);
    }
}

bool Room::Private::hasMember(User* u) const
{
    return membersMap.values(u->name()).contains(u);
}

User* Room::Private::member(QString id) const
{
    User* u = connection->user(id);
    return hasMember(u) ? u : nullptr;
}

void Room::Private::renameMember(User* u, QString oldName)
{
    if (hasMember(u))
    {
        qWarning() << "Room::Private::renameMember(): the user "
                   << u->name()
                   << "is already known in the room under a new name.";
        return;
    }

    if (membersMap.values(oldName).contains(u))
    {
        removeMemberFromMap(oldName, u);
        insertMemberIntoMap(u);
        emit q->memberRenamed(u);

        updateDisplayname();
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

void Room::userRenamed(User* user, QString oldName)
{
    d->renameMember(user, oldName);
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
//        qWarning()
//            << "Room::roomMemberName(): user" << u->id()
//            << "is not a member of the room" << id();
//    }

    // In case of more than one namesake, disambiguate with user id.
    return username % " (" % u->id() % ")";
}

QString Room::roomMembername(QString userId) const
{
    return roomMembername(connection()->user(userId));
}

void Room::updateData(SyncRoomData& data)
{
    if( d->prevBatch.isEmpty() )
        d->prevBatch = data.timelinePrevBatch;
    setJoinState(data.joinState);

    processStateEvents(data.state);

    // State changes can arrive in a timeline event; so check those.
    processStateEvents(data.timeline);
    addNewMessageEvents(data.timeline.release());

    for( Event* ephemeralEvent: data.ephemeral )
    {
        processEphemeralEvent(ephemeralEvent);
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

void Room::postMessage(QString type, QString content)
{
    connection()->callApi<PostMessageJob>(id(), type, content);
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

Connection* Room::connection() const
{
    return d->connection;
}

void Room::addNewMessageEvents(Events events)
{
    d->dropDuplicateEvents(events);
    if (events.empty())
        return;
    emit aboutToAddNewMessages(events);
    doAddNewMessageEvents(events);
    emit addedMessages();
}

bool Room::Private::isEventNotable(const Event* e) const
{
    return e->senderId() != connection->userId() &&
            e->type() == EventType::RoomMessage;
}

void Room::doAddNewMessageEvents(const Events& events)
{
    Q_ASSERT(!events.isEmpty());
    Timeline::size_type newUnreadMessages = 0;

    // The first event in the batch defines whose read marker we can
    // automatically promote any further (if this author has read the whole
    // timeline by then). Others will need explicit read receipts
    // from the server (or, for the local user, markMessagesAsRead() invocation)
    // to promote their read markers over the new message events.
    User* firstWriter = connection()->user(events.front()->senderId());
    bool canAutoPromote = d->readMarker(firstWriter) == d->timeline.crbegin();

    for (auto e: events)
    {
        d->appendEvent(e);
        newUnreadMessages += d->isEventNotable(e);
    }
    qDebug() << "Added" << events.size()
             << "(with" << newUnreadMessages << "notable)"
             << "new events to room" << displayName();
    if (canAutoPromote)
    {
        qDebug() << "Auto-promoting" << firstWriter->id() << "over own events";
        d->promoteReadMarker(firstWriter, events.front()->id());
    }

    if( !d->unreadMessages && newUnreadMessages > 0)
    {
        d->unreadMessages = true;
        emit unreadMessagesChanged(this);
        qDebug() << "Room" << displayName() << "has unread messages";
    }
}

void Room::addHistoricalMessageEvents(Events events)
{
    d->dropDuplicateEvents(events);
    if (events.empty())
        return;
    emit aboutToAddHistoricalMessages(events);
    doAddHistoricalMessageEvents(events);
    emit addedMessages();
}

void Room::doAddHistoricalMessageEvents(const Events& events)
{
    Q_ASSERT(!events.isEmpty());
    // Historical messages arrive in newest-to-oldest order
    for (auto e: events)
        d->prependEvent(e);
    qDebug() << "Added" << events.size() << "historical events to room"
             << displayName();
}

void Room::processStateEvents(const Events& events)
{
    for (auto event: events)
    {
        if( event->type() == EventType::RoomName )
        {
            RoomNameEvent* nameEvent = static_cast<RoomNameEvent*>(event);
            d->name = nameEvent->name();
            qDebug() << "room name:" << d->name;
            d->updateDisplayname();
            emit namesChanged(this);
        }
        if( event->type() == EventType::RoomAliases )
        {
            RoomAliasesEvent* aliasesEvent = static_cast<RoomAliasesEvent*>(event);
            d->aliases = aliasesEvent->aliases();
            qDebug() << "room aliases:" << d->aliases;
            // No displayname update - aliases are not used to render a displayname
            emit namesChanged(this);
        }
        if( event->type() == EventType::RoomCanonicalAlias )
        {
            RoomCanonicalAliasEvent* aliasEvent = static_cast<RoomCanonicalAliasEvent*>(event);
            d->canonicalAlias = aliasEvent->alias();
            qDebug() << "room canonical alias:" << d->canonicalAlias;
            d->updateDisplayname();
            emit namesChanged(this);
        }
        if( event->type() == EventType::RoomTopic )
        {
            RoomTopicEvent* topicEvent = static_cast<RoomTopicEvent*>(event);
            d->topic = topicEvent->topic();
            emit topicChanged();
        }
        if( event->type() == EventType::RoomMember )
        {
            RoomMemberEvent* memberEvent = static_cast<RoomMemberEvent*>(event);
            // Can't use d->member() below because the user may be not a member (yet)
            User* u = d->connection->user(memberEvent->userId());
            u->processEvent(event);
            if( memberEvent->membership() == MembershipType::Join )
            {
                d->addMember(u);
            }
            else if( memberEvent->membership() == MembershipType::Leave )
            {
                d->removeMember(u);
            }
        }
    }
}

void Room::processEphemeralEvent(Event* event)
{
    if( event->type() == EventType::Typing )
    {
        TypingEvent* typingEvent = static_cast<TypingEvent*>(event);
        d->usersTyping.clear();
        for( const QString& userId: typingEvent->users() )
        {
            if (auto m = d->member(userId))
                d->usersTyping.append(m);
        }
        emit typingChanged();
    }
    if( event->type() == EventType::Receipt )
    {
        auto receiptEvent = static_cast<ReceiptEvent*>(event);
        for( const auto &eventReceiptPair: receiptEvent->events() )
        {
            const auto& eventId = eventReceiptPair.first;
            const auto& receipts = eventReceiptPair.second;
            {
                auto qd = qDebug() << "Marking event" << eventId << "as read for";
                if (receipts.size() == 1)
                    qd << receipts[0].userId;
                else
                    qd << receipts.size() << "users";
            }
            for( const Receipt& r: receipts )
                if (auto m = d->member(r.userId))
                    d->promoteReadMarker(m, eventId);
        }
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
            return u2 == connection->user() ||
                    (u1 != connection->user() && u1->id() < u2->id());
        }
    );

    // i. One-on-one chat. first_two[1] == connection->user() in this case.
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
        // The below two lines extend the spec. They take care of the case
        // when there are two rooms with the same name.
        // The format is unwittingly borrowed from the email address format.
        if (!canonicalAlias.isEmpty())
            return name % " <" % canonicalAlias % ">";

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
    return n1 < n2;
}

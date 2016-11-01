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

#ifndef QMATRIXCLIENT_ROOM_H
#define QMATRIXCLIENT_ROOM_H

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QObject>
#include <QtCore/QJsonObject>

#include "jobs/syncjob.h"
#include "joinstate.h"

namespace QMatrixClient
{
    class Event;
    class Connection;
    class User;
    class MemberSorter;

    class Room: public QObject
    {
            Q_OBJECT
        public:
            using Timeline = Owning<Events>;

            Room(Connection* connection, QString id);
            virtual ~Room();

            Q_INVOKABLE QString id() const;
            Q_INVOKABLE const Timeline& messageEvents() const;
            Q_INVOKABLE QString name() const;
            Q_INVOKABLE QStringList aliases() const;
            Q_INVOKABLE QString canonicalAlias() const;
            Q_INVOKABLE QString displayName() const;
            Q_INVOKABLE QString topic() const;
            Q_INVOKABLE JoinState joinState() const;
            Q_INVOKABLE QList<User*> usersTyping() const;
            QList<User*> membersLeft() const;

            Q_INVOKABLE QList<User*> users() const;

            /**
             * @brief Produces a disambiguated name for a given user in
             * the context of the room
             */
            Q_INVOKABLE QString roomMembername(User* u) const;
            /**
             * @brief Produces a disambiguated name for a user with this id in
             * the context of the room
             */
            Q_INVOKABLE QString roomMembername(QString userId) const;

            Q_INVOKABLE void updateData(SyncRoomData& data );
            Q_INVOKABLE void setJoinState( JoinState state );

            Q_INVOKABLE QString lastReadEvent(User* user);
            /**
             * @brief Mark the message at the iterator as read
             *
             * Marks the message at the iterator as read; also posts a read
             * receipt to the server either for this message or, if it's from
             * the local user, for the nearest non-local message before.
             */
            Q_INVOKABLE void markMessagesAsRead(Timeline::const_iterator last);
            /**
             * @brief Mark the most recent message in the timeline as read
             *
             * This effectively marks everything in the room as read.
             */
            Q_INVOKABLE void markMessagesAsRead();

            Q_INVOKABLE bool hasUnreadMessages();

            Q_INVOKABLE int notificationCount() const;
            Q_INVOKABLE void resetNotificationCount();
            Q_INVOKABLE int highlightCount() const;
            Q_INVOKABLE void resetHighlightCount();

            MemberSorter memberSorter() const;

        public slots:
            void getPreviousContent();
            void userRenamed(User* user, QString oldName);

        signals:
            void aboutToAddHistoricalMessages(const Events& events);
            void aboutToAddNewMessages(const Events& events);
            void addedMessages();

            /**
             * @brief The room name, the canonical alias or other aliases changed
             *
             * Not triggered when displayname changes.
             */
            void namesChanged(Room* room);
            /** @brief The room displayname changed */
            void displaynameChanged(Room* room);
            void topicChanged();
            void userAdded(User* user);
            void userRemoved(User* user);
            void memberRenamed(User* user);
            void joinStateChanged(JoinState oldState, JoinState newState);
            void typingChanged();
            void highlightCountChanged(Room* room);
            void notificationCountChanged(Room* room);
            void lastReadEventChanged(User* user);
            void unreadMessagesChanged(Room* room);

        protected:
            Connection* connection() const;
            virtual void doAddNewMessageEvents(const Events& events);
            virtual void doAddHistoricalMessageEvents(const Events& events);
            virtual void processStateEvents(const Events& events);
            virtual void processEphemeralEvent(Event* event);

            bool promoteReadMarker(QString newLastReadEventId);

        private:
            class Private;
            Private* d;

            void addNewMessageEvents(const Events& events);
            void addHistoricalMessageEvents(const Events& events);

            void setLastReadEvent(User* user, QString eventId);
    };

    class MemberSorter
    {
        public:
            MemberSorter(const Room* r) : room(r) { }

            bool operator()(User* u1, User* u2) const;

            template <typename ContT>
            typename ContT::size_type lowerBoundIndex(const ContT& c,
                                                      typename ContT::value_type v) const
            {
                return  std::lower_bound(c.begin(), c.end(), v, *this) - c.begin();
            }

        private:
            const Room* room;
    };
}

#endif // QMATRIXCLIENT_ROOM_H

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

    class Room: public QObject
    {
            Q_OBJECT
        public:
            using Timeline = Events;

            Room(Connection* connection, QString id);
            virtual ~Room();

            Q_INVOKABLE QString id() const;
            Q_INVOKABLE Timeline messageEvents() const;
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

            Q_INVOKABLE void updateData( const SyncRoomData& data );
            Q_INVOKABLE void setJoinState( JoinState state );

            Q_INVOKABLE void markMessageAsRead( Event* event );
            Q_INVOKABLE QString lastReadEvent(User* user);

            Q_INVOKABLE int notificationCount() const;
            Q_INVOKABLE void resetNotificationCount();
            Q_INVOKABLE int highlightCount() const;
            Q_INVOKABLE void resetHighlightCount();

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

        protected:
            Connection* connection() const;
            virtual void doAddNewMessageEvents(const Events& events);
            virtual void doAddHistoricalMessageEvents(const Events& events);
            virtual void processStateEvents(const Events& events);
            virtual void processEphemeralEvent(Event* event);

        private:
            class Private;
            Private* d;

            void addNewMessageEvents(const Events& events);
            void addHistoricalMessageEvents(const Events& events);
    };
}

#endif // QMATRIXCLIENT_ROOM_H

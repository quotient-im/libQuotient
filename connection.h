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

#pragma once

#include "serverapi/call.h"

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QSize>

#include <utility>

namespace QMatrixClient
{
    class Room;
    class User;
    class Event;
    class ConnectionPrivate;
    class ConnectionData;

    class SyncJob;
    class RoomMessagesJob;
    class MediaThumbnailJob;
    class JoinRoomJob;

    class Connection: public QObject {
            Q_OBJECT
        public:
            Connection(QUrl server, QObject* parent = nullptr);
            Connection();
            virtual ~Connection();

            QHash<QString, Room*> roomMap() const;
            Q_INVOKABLE virtual bool isConnected();

            Q_INVOKABLE virtual void resolveServer( QString domain );
            Q_INVOKABLE virtual void connectToServer( QString user, QString password );
            Q_INVOKABLE virtual void connectWithToken( QString userId, QString token );
            Q_INVOKABLE virtual void reconnect();
            Q_INVOKABLE virtual void disconnectFromServer();
            Q_INVOKABLE virtual void logout();

            Q_INVOKABLE virtual void sync(int timeout=-1);
            Q_INVOKABLE virtual void stopSync();
            /** @deprecated Use callApi<PostMessageJob>() or Room::postMessage() instead */
            Q_INVOKABLE virtual void postMessage( Room* room, QString type, QString message );
            Q_INVOKABLE virtual JoinRoomJob* joinRoom( QString roomAlias );
            Q_INVOKABLE virtual void leaveRoom( Room* room );
            Q_INVOKABLE virtual RoomMessagesJob* getMessages( Room* room, QString from );

            Q_INVOKABLE QUrl homeserver() const;
            Q_INVOKABLE User* user(QString userId);
            Q_INVOKABLE User* user();
            Q_INVOKABLE QString userId() const;
            /** @deprecated Use accessToken() instead. */
            Q_INVOKABLE QString token() const;
            Q_INVOKABLE QString accessToken() const;
            Q_INVOKABLE SyncJob* syncJob() const;
            Q_INVOKABLE int millisToReconnect() const;

            template <typename JobT, typename... JobArgTs>
            JobT* callApi(JobArgTs... jobArgs)
            {
                auto job = new JobT(connectionData(), jobArgs...);
                job->start();
                return job;
            }

            template <typename ConfigT>
            ServerApi::PendingCall<ConfigT>* callServer(const ConfigT& c)
            {
                return new ServerApi::PendingCall<ConfigT>(connectionData(), c);
            }

        signals:
            void resolved();
            void connected();
            void reconnected();
            void loggedOut();

            void syncDone();
            void newRoom(Room* room);
            void joinedRoom(Room* room);

            void loginError(QString error);
            void networkError(size_t nextAttempt, int inMilliseconds);
            void resolveError(QString error);
            void syncError(QString error);
            //void jobError(BaseJob* job);

        protected:
            /**
             * @brief Access the underlying ConnectionData class
             */
            ConnectionData* connectionData();

            /**
             * @brief Find a (possibly new) Room object for the specified id
             * Use this method whenever you need to find a Room object in
             * the local list of rooms. Note that this does not interact with
             * the server; in particular, does not automatically create rooms
             * on the server.
             * @return a pointer to a Room object with the specified id; nullptr
             * if roomId is empty if createRoom() failed to create a Room object.
             */
            Room* provideRoom(QString roomId);

            /**
             * makes it possible for derived classes to have its own User class
             */
            virtual User* createUser(QString userId);

            /**
             * makes it possible for derived classes to have its own Room class
             */
            virtual Room* createRoom(QString roomId);

        private:
            class Private;
            Private* d;
    };
}

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

#ifndef QMATRIXCLIENT_CONNECTION_H
#define QMATRIXCLIENT_CONNECTION_H

#include <QtCore/QObject>

namespace QMatrixClient
{
    class Room;
    class User;
    class Event;
    class ConnectionPrivate;
    class ConnectionData;

    class SyncJob;
    class RoomMessagesJob;
    class PostReceiptJob;
    class MediaThumbnailJob;

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
            Q_INVOKABLE virtual SyncJob* sync(int timeout=-1);
            Q_INVOKABLE virtual void postMessage( Room* room, QString type, QString message );
            Q_INVOKABLE virtual PostReceiptJob* postReceipt( Room* room, Event* event );
            Q_INVOKABLE virtual void joinRoom( QString roomAlias );
            Q_INVOKABLE virtual void leaveRoom( Room* room );
            Q_INVOKABLE virtual void getMembers( Room* room );
            Q_INVOKABLE virtual RoomMessagesJob* getMessages( Room* room, QString from );
            virtual MediaThumbnailJob* getThumbnail( QUrl url, int requestedWidth, int requestedHeight );

            Q_INVOKABLE virtual User* user(QString userId);
            Q_INVOKABLE virtual User* user();
            Q_INVOKABLE virtual QString userId();
            Q_INVOKABLE virtual QString token();

        signals:
            void connected();
            void reconnected();
            void resolved();
            void syncStarted();
            void syncDone();
            void newRoom(Room* room);
            void joinedRoom(Room* room);

            void loginError(QString error);
            void connectionError(QString error);
            void resolveError(QString error);
            //void jobError(BaseJob* job);
            
        protected:
            /**
             * Access the underlying ConnectionData class
             */
            ConnectionData* connectionData();
            
            /**
             * makes it possible for derived classes to have its own User class
             */
            virtual User* createUser(QString userId);
            
            /**
             * makes it possible for derived classes to have its own Room class
             */
            virtual Room* createRoom(QString roomId);

        private:
            friend class ConnectionPrivate;
            ConnectionPrivate* d;
    };
}

#endif // QMATRIXCLIENT_CONNECTION_H

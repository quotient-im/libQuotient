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

#include "jobs/generated/leaving.h"
#include "joinstate.h"

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QSize>

#include <functional>

namespace QMatrixClient
{
    class Room;
    class User;
    class RoomEvent;
    class ConnectionPrivate;
    class ConnectionData;

    class SyncJob;
    class SyncData;
    class RoomMessagesJob;
    class PostReceiptJob;
    class MediaThumbnailJob;
    class JoinRoomJob;

    class Connection: public QObject {
            Q_OBJECT

            /** Whether or not the rooms state should be cached locally
             * \sa loadState(), saveState()
             */
            Q_PROPERTY(bool cacheState READ cacheState WRITE setCacheState NOTIFY cacheStateChanged)
        public:
            using room_factory_t =
                std::function<Room*(Connection*, const QString&, JoinState joinState)>;
            using user_factory_t =
                std::function<User*(Connection*, const QString&)>;

            explicit Connection(const QUrl& server, QObject* parent = nullptr);
            Connection();
            virtual ~Connection();

            QHash<QPair<QString, bool>, Room*> roomMap() const;

            Q_INVOKABLE virtual void resolveServer(const QString& domain);
            Q_INVOKABLE virtual void connectToServer(const QString& user,
                                                     const QString& password);
            Q_INVOKABLE virtual void connectWithToken(const QString& userId,
                                                      const QString& token);
            Q_INVOKABLE virtual void reconnect();
            /** @deprecated Use stopSync() instead */
            Q_INVOKABLE virtual void disconnectFromServer() { stopSync(); }
            Q_INVOKABLE virtual void logout();

            Q_INVOKABLE void sync(int timeout = -1);
            Q_INVOKABLE void stopSync();

            // Old API that will be abolished any time soon. DO NOT USE.

            /** @deprecated Use callApi<PostMessageJob>() or Room::postMessage() instead */
            Q_INVOKABLE virtual void postMessage(Room* room, const QString& type,
                                                 const QString& message) const;
            /** @deprecated Use callApi<PostReceiptJob>() or Room::postReceipt() instead */
            Q_INVOKABLE virtual PostReceiptJob* postReceipt(Room* room,
                                                            RoomEvent* event) const;
            /** @deprecated Use callApi<JoinRoomJob>() instead */
            Q_INVOKABLE virtual JoinRoomJob* joinRoom(const QString& roomAlias);
            /** @deprecated Use callApi<LeaveRoomJob>() or Room::leaveRoom() instead */
            Q_INVOKABLE virtual void leaveRoom( Room* room );
            /** @deprecated User callApi<RoomMessagesJob>() or Room::getPreviousContent() instead */
            Q_INVOKABLE virtual RoomMessagesJob* getMessages(Room* room,
                                                             const QString& from) const;
            /** @deprecated Use callApi<MediaThumbnailJob>() instead */
            virtual MediaThumbnailJob* getThumbnail(const QUrl& url,
                                                    QSize requestedSize) const;
            /** @deprecated Use callApi<MediaThumbnailJob>() instead */
            MediaThumbnailJob* getThumbnail(const QUrl& url, int requestedWidth,
                                            int requestedHeight) const;
            /** Sends /forget to the server and also deletes room locally.
             * This method is in Connection, not in Room, since it's a
             * room lifecycle operation, and Connection is an acting room manager.
             * It ensures that the local user is not a member of a room (running /leave,
             * if necessary) then issues a /forget request and if that one doesn't fail
             * deletion of the local Room object is ensured.
             * \param id - the room id to forget
             * \return - the ongoing /forget request to the server; note that the
             * success() signal of this request is connected to deleteLater()
             * of a respective room so by the moment this finishes, there might be no
             * Room object anymore.
             */
            ForgetRoomJob* forgetRoom(const QString& id) const;

            Q_INVOKABLE QUrl homeserver() const;
            Q_INVOKABLE User* user(const QString& userId);
            Q_INVOKABLE User* user();
            Q_INVOKABLE QString userId() const;
            /** @deprecated Use accessToken() instead. */
            Q_INVOKABLE QString token() const;
            Q_INVOKABLE QString accessToken() const;
            Q_INVOKABLE SyncJob* syncJob() const;
            Q_INVOKABLE int millisToReconnect() const;

            /**
             * Call this before first sync to load from previously saved file.
             *
             * \param fromFile A local path to read the state from. Uses QUrl
             * to be QML-friendly. Empty parameter means using a path
             * defined by stateCachePath().
             */
            Q_INVOKABLE void loadState(const QUrl &fromFile = {});
            /**
             * This method saves the current state of rooms (but not messages
             * in them) to a local cache file, so that it could be loaded by
             * loadState() on a next run of the client.
             *
             * \param toFile A local path to save the state to. Uses QUrl to be
             * QML-friendly. Empty parameter means using a path defined by
             * stateCachePath().
             */
            Q_INVOKABLE void saveState(const QUrl &toFile = {}) const;

            /**
             * The default path to store the cached room state, defined as
             * follows:
             *     QStandardPaths::writeableLocation(QStandardPaths::CacheLocation) + _safeUserId + "_state.json"
             * where `_safeUserId` is userId() with `:` (colon) replaced with
             * `_` (underscore)
             * /see loadState(), saveState()
             */
            Q_INVOKABLE QString stateCachePath() const;

            bool cacheState() const;
            void setCacheState(bool newValue);

            /**
             * This is a universal method to start a job of a type passed
             * as a template parameter. Arguments to callApi() are arguments
             * to the job constructor _except_ the first ConnectionData*
             * argument - callApi() will pass it automatically.
             */
            template <typename JobT, typename... JobArgTs>
            JobT* callApi(JobArgTs&&... jobArgs) const
            {
                auto job = new JobT(std::forward<JobArgTs>(jobArgs)...);
                job->start(connectionData());
                return job;
            }

            /** Generates a new transaction id. Transaction id's are unique within
             * a single Connection object
             */
            Q_INVOKABLE QByteArray generateTxnId();

            template <typename T = Room>
            static void setRoomType()
            {
                createRoom =
                    [](Connection* c, const QString& id, JoinState joinState)
                    { return new T(c, id, joinState); };
            }

            template <typename T = User>
            static void setUserType()
            {
                createUser =
                    [](Connection* c, const QString& id) { return new T(id, c); };
            }

        signals:
            void resolved();
            void connected();
            void reconnected();
            void loggedOut();

            void syncDone();
            void newRoom(Room* room);
            void invitedRoom(Room* room, Room* prev);
            void joinedRoom(Room* room, Room* prev);
            void leftRoom(Room* room, Room* prev);
            void aboutToDeleteRoom(Room* room);

            void loginError(QString error);
            void networkError(size_t nextAttempt, int inMilliseconds);
            void resolveError(QString error);
            void syncError(QString error);
            //void jobError(BaseJob* job);

            void cacheStateChanged();

        protected:
            /**
             * @brief Access the underlying ConnectionData class
             */
            const ConnectionData* connectionData() const;

            /**
             * @brief Find a (possibly new) Room object for the specified id
             * Use this method whenever you need to find a Room object in
             * the local list of rooms. Note that this does not interact with
             * the server; in particular, does not automatically create rooms
             * on the server.
             * @return a pointer to a Room object with the specified id; nullptr
             * if roomId is empty if createRoom() failed to create a Room object.
             */
            Room* provideRoom(const QString& roomId, JoinState joinState);


            /**
             * Completes loading sync data.
             */
            void onSyncSuccess(SyncData &&data);

        private:
            class Private;
            Private* d;

            static room_factory_t createRoom;
            static user_factory_t createUser;
    };
}  // namespace QMatrixClient

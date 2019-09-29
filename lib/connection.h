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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include "joinstate.h"
#include "qt_connection_util.h"

#include "csapi/create_room.h"

#include "events/accountdataevents.h"

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QUrl>

#include <functional>

namespace QtOlm {
class Account;
}

namespace Quotient {
Q_NAMESPACE

class Room;
class User;
class ConnectionData;
class RoomEvent;

class SyncJob;
class SyncData;
class RoomMessagesJob;
class PostReceiptJob;
class ForgetRoomJob;
class MediaThumbnailJob;
class JoinRoomJob;
class UploadContentJob;
class GetContentJob;
class DownloadFileJob;
class SendToDeviceJob;
class SendMessageJob;
class LeaveRoomJob;

class Connection;

using room_factory_t =
    std::function<Room*(Connection*, const QString&, JoinState)>;
using user_factory_t = std::function<User*(Connection*, const QString&)>;

/** The default factory to create room objects
 *
 * Just a wrapper around operator new.
 * \sa Connection::setRoomFactory, Connection::setRoomType
 */
template <typename T = Room>
static inline room_factory_t defaultRoomFactory()
{
    return [](Connection* c, const QString& id, JoinState js) {
        return new T(c, id, js);
    };
}

/** The default factory to create user objects
 *
 * Just a wrapper around operator new.
 * \sa Connection::setUserFactory, Connection::setUserType
 */
template <typename T = User>
static inline user_factory_t defaultUserFactory()
{
    return [](Connection* c, const QString& id) { return new T(id, c); };
}

/** Enumeration with flags defining the network job running policy
 * So far only background/foreground flags are available.
 *
 * \sa Connection::callApi, Connection::run
 */
enum RunningPolicy { ForegroundRequest = 0x0, BackgroundRequest = 0x1 };

Q_ENUM_NS(RunningPolicy)

class Connection : public QObject {
    Q_OBJECT

    Q_PROPERTY(User* localUser READ user NOTIFY stateChanged)
    Q_PROPERTY(QString localUserId READ userId NOTIFY stateChanged)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY stateChanged)
    Q_PROPERTY(QByteArray accessToken READ accessToken NOTIFY stateChanged)
    Q_PROPERTY(QString defaultRoomVersion READ defaultRoomVersion NOTIFY
                   capabilitiesLoaded)
    Q_PROPERTY(QUrl homeserver READ homeserver WRITE setHomeserver NOTIFY
                   homeserverChanged)
    Q_PROPERTY(QString domain READ domain NOTIFY homeserverChanged)
    Q_PROPERTY(bool cacheState READ cacheState WRITE setCacheState NOTIFY
                   cacheStateChanged)
    Q_PROPERTY(bool lazyLoading READ lazyLoading WRITE setLazyLoading NOTIFY
                   lazyLoadingChanged)

public:
    // Room ids, rather than room pointers, are used in the direct chat
    // map types because the library keeps Invite rooms separate from
    // rooms in Join and Leave state; and direct chats in account data
    // are stored with no regard to their state.
    using DirectChatsMap = QMultiHash<const User*, QString>;
    using DirectChatUsersMap = QMultiHash<QString, User*>;
    using IgnoredUsersList = IgnoredUsersEvent::content_type;

    using UsersToDevicesToEvents =
        UnorderedMap<QString, UnorderedMap<QString, const Event&>>;

    enum RoomVisibility {
        PublishRoom,
        UnpublishRoom
    }; // FIXME: Should go inside CreateRoomJob

    explicit Connection(QObject* parent = nullptr);
    explicit Connection(const QUrl& server, QObject* parent = nullptr);
    ~Connection() override;

    /// Get all Invited and Joined rooms
    /*!
     * \return a hashmap from a composite key - room name and whether
     *         it's an Invite rather than Join - to room pointers
     * \sa allRooms, rooms, roomsWithTag
     */
    [[deprecated("Use allRooms(), roomsWithTag() or rooms(joinStates) instead")]]
    QHash<QPair<QString, bool>, Room*> roomMap() const;

    /// Get all rooms known within this Connection
    /*!
     * This includes Invite, Join and Leave rooms, in no particular order.
     * \note Leave rooms will only show up in the list if they have been left
     *       in the same running session. The library doesn't cache left rooms
     *       between runs and it doesn't retrieve the full list of left rooms
     *       from the server.
     * \sa rooms, room, roomsWithTag
     */
    Q_INVOKABLE QVector<Room*> allRooms() const;

    /// Get rooms that have either of the given join state(s)
    /*!
     * This method returns, in no particular order, rooms which join state
     * matches the mask passed in \p joinStates.
     * \note Similar to allRooms(), this won't retrieve the full list of
     *       Leave rooms from the server.
     * \sa allRooms, room, roomsWithTag
     */
    Q_INVOKABLE QVector<Room*> rooms(JoinStates joinStates) const;

    /// Get the total number of rooms in the given join state(s)
    Q_INVOKABLE int roomsCount(JoinStates joinStates) const;

    /** Check whether the account has data of the given type
     * Direct chats map is not supported by this method _yet_.
     */
    bool hasAccountData(const QString& type) const;

    /** Get a generic account data event of the given type
     * This returns an account data event of the given type
     * stored on the server. Direct chats map cannot be retrieved
     * using this method _yet_; use directChats() instead.
     */
    const EventPtr& accountData(const QString& type) const;

    /** Get a generic account data event of the given type
     * This returns an account data event of the given type
     * stored on the server. Direct chats map cannot be retrieved
     * using this method _yet_; use directChats() instead.
     */
    template <typename EventT>
    const typename EventT::content_type accountData() const
    {
        if (const auto& eventPtr = accountData(EventT::matrixTypeId()))
            return eventPtr->content();
        return {};
    }

    /** Get account data as a JSON object
     * This returns the content part of the account data event
     * of the given type. Direct chats map cannot be retrieved using
     * this method _yet_; use directChats() instead.
     */
    Q_INVOKABLE QJsonObject accountDataJson(const QString& type) const;

    /** Set a generic account data event of the given type */
    void setAccountData(EventPtr&& event);

    Q_INVOKABLE void setAccountData(const QString& type,
                                    const QJsonObject& content);

    /** Get all Invited and Joined rooms grouped by tag
     * \return a hashmap from tag name to a vector of room pointers,
     *         sorted by their order in the tag - details are at
     *         https://matrix.org/speculator/spec/drafts%2Fe2e/client_server/unstable.html#id95
     */
    QHash<QString, QVector<Room*>> tagsToRooms() const;

    /** Get all room tags known on this connection */
    QStringList tagNames() const;

    /** Get the list of rooms with the specified tag */
    QVector<Room*> roomsWithTag(const QString& tagName) const;

    /** Mark the room as a direct chat with the user
     * This function marks \p room as a direct chat with \p user.
     * Emits the signal synchronously, without waiting to complete
     * synchronisation with the server.
     *
     * \sa directChatsListChanged
     */
    void addToDirectChats(const Room* room, User* user);

    /** Unmark the room from direct chats
     * This function removes the room id from direct chats either for
     * a specific \p user or for all users if \p user in nullptr.
     * The room id is used to allow removal of, e.g., ids of forgotten
     * rooms; a Room object need not exist. Emits the signal
     * immediately, without waiting to complete synchronisation with
     * the server.
     *
     * \sa directChatsListChanged
     */
    void removeFromDirectChats(const QString& roomId, User* user = nullptr);

    /** Check whether the room id corresponds to a direct chat */
    bool isDirectChat(const QString& roomId) const;

    /** Get the whole map from users to direct chat rooms */
    DirectChatsMap directChats() const;

    /** Retrieve the list of users the room is a direct chat with
     * @return The list of users for which this room is marked as
     * a direct chat; an empty list if the room is not a direct chat
     */
    QList<User*> directChatUsers(const Room* room) const;

    /** Check whether a particular user is in the ignore list */
    Q_INVOKABLE bool isIgnored(const User* user) const;

    /** Get the whole list of ignored users */
    Q_INVOKABLE IgnoredUsersList ignoredUsers() const;

    /** Add the user to the ignore list
     * The change signal is emitted synchronously, without waiting
     * to complete synchronisation with the server.
     *
     * \sa ignoredUsersListChanged
     */
    Q_INVOKABLE void addToIgnoredUsers(const User* user);

    /** Remove the user from the ignore list */
    /** Similar to adding, the change signal is emitted synchronously.
     *
     * \sa ignoredUsersListChanged
     */
    Q_INVOKABLE void removeFromIgnoredUsers(const User* user);

    /** Get the full list of users known to this account */
    QMap<QString, User*> users() const;

    /** Get the base URL of the homeserver to connect to */
    QUrl homeserver() const;
    /** Get the domain name used for ids/aliases on the server */
    QString domain() const;
    /** Find a room by its id and a mask of applicable states */
    Q_INVOKABLE Room* room(const QString& roomId,
                           JoinStates states = JoinState::Invite
                                               | JoinState::Join) const;
    /** Find a room by its alias and a mask of applicable states */
    Q_INVOKABLE Room* roomByAlias(const QString& roomAlias,
                                  JoinStates states = JoinState::Invite
                                                      | JoinState::Join) const;
    /** Update the internal map of room aliases to IDs */
    /// This is used to maintain the internal index of room aliases.
    /// It does NOT change aliases on the server,
    /// \sa Room::setLocalAliases
    void updateRoomAliases(const QString& roomId, const QString& aliasServer,
                           const QStringList& previousRoomAliases,
                           const QStringList& roomAliases);
    Q_INVOKABLE Room* invitation(const QString& roomId) const;
    Q_INVOKABLE User* user(const QString& userId);
    const User* user() const;
    User* user();
    QString userId() const;
    QString deviceId() const;
    QByteArray accessToken() const;
    QtOlm::Account* olmAccount() const;
    Q_INVOKABLE SyncJob* syncJob() const;
    Q_INVOKABLE int millisToReconnect() const;

    Q_INVOKABLE void getTurnServers();

    struct SupportedRoomVersion {
        QString id;
        QString status;

        static const QString StableTag; // "stable", as of CS API 0.5
        bool isStable() const { return status == StableTag; }

        friend QDebug operator<<(QDebug dbg, const SupportedRoomVersion& v)
        {
            QDebugStateSaver _(dbg);
            return dbg.nospace() << v.id << '/' << v.status;
        }
    };

    /// Get the room version recommended by the server
    /** Only works after server capabilities have been loaded.
     * \sa loadingCapabilities */
    QString defaultRoomVersion() const;
    /// Get the room version considered stable by the server
    /** Only works after server capabilities have been loaded.
     * \sa loadingCapabilities */
    QStringList stableRoomVersions() const;
    /// Get all room versions supported by the server
    /** Only works after server capabilities have been loaded.
     * \sa loadingCapabilities */
    QVector<SupportedRoomVersion> availableRoomVersions() const;

    /**
     * Call this before first sync to load from previously saved file.
     *
     * \param fromFile A local path to read the state from. Uses QUrl
     * to be QML-friendly. Empty parameter means saving to the directory
     * defined by stateCachePath() / stateCacheDir().
     */
    Q_INVOKABLE void loadState();
    /**
     * This method saves the current state of rooms (but not messages
     * in them) to a local cache file, so that it could be loaded by
     * loadState() on a next run of the client.
     *
     * \param toFile A local path to save the state to. Uses QUrl to be
     * QML-friendly. Empty parameter means saving to the directory
     * defined by stateCachePath() / stateCacheDir().
     */
    Q_INVOKABLE void saveState() const;

    /// This method saves the current state of a single room.
    void saveRoomState(Room* r) const;

    /// Get the default directory path to save the room state to
    /** \sa stateCacheDir */
    Q_INVOKABLE QString stateCachePath() const;

    /// Get the default directory to save the room state to
    /**
     * This function returns the default directory to store the cached
     * room state, defined as follows:
     * \code
     *     QStandardPaths::writeableLocation(QStandardPaths::CacheLocation) +
     * _safeUserId + "_state.json" \endcode where `_safeUserId` is userId() with
     * `:` (colon) replaced by
     * `_` (underscore), as colons are reserved characters on Windows.
     * \sa loadState, saveState, stateCachePath
     */
    QDir stateCacheDir() const;

    /** Whether or not the rooms state should be cached locally
     * \sa loadState(), saveState()
     */
    bool cacheState() const;
    void setCacheState(bool newValue);

    bool lazyLoading() const;
    void setLazyLoading(bool newValue);

    /*! Start a pre-created job object on this connection */
    void run(BaseJob* job, RunningPolicy runningPolicy = ForegroundRequest) const;

    /*! Start a job of a specified type with specified arguments and policy
     *
     * This is a universal method to create and start a job of a type passed
     * as a template parameter. The policy allows to fine-tune the way
     * the job is executed - as of this writing it means a choice
     * between "foreground" and "background".
     *
     * \param runningPolicy controls how the job is executed
     * \param jobArgs arguments to the job constructor
     *
     * \sa BaseJob::isBackground. QNetworkRequest::BackgroundRequestAttribute
     */
    template <typename JobT, typename... JobArgTs>
    JobT* callApi(RunningPolicy runningPolicy, JobArgTs&&... jobArgs) const
    {
        auto job = new JobT(std::forward<JobArgTs>(jobArgs)...);
        run(job, runningPolicy);
        return job;
    }

    /*! Start a job of a specified type with specified arguments
     *
     * This is an overload that runs the job with "foreground" policy.
     */
    template <typename JobT, typename... JobArgTs>
    JobT* callApi(JobArgTs&&... jobArgs) const
    {
        return callApi<JobT>(ForegroundRequest,
                             std::forward<JobArgTs>(jobArgs)...);
    }

    /** Generate a new transaction id. Transaction id's are unique within
     * a single Connection object
     */
    Q_INVOKABLE QByteArray generateTxnId() const;

    /// Set a room factory function
    static void setRoomFactory(room_factory_t f);

    /// Set a user factory function
    static void setUserFactory(user_factory_t f);

    /// Get a room factory function
    static room_factory_t roomFactory();

    /// Get a user factory function
    static user_factory_t userFactory();

    /// Set the room factory to default with the overriden room type
    template <typename T>
    static void setRoomType()
    {
        setRoomFactory(defaultRoomFactory<T>());
    }

    /// Set the user factory to default with the overriden user type
    template <typename T>
    static void setUserType()
    {
        setUserFactory(defaultUserFactory<T>());
    }

public slots:
    /** Set the homeserver base URL */
    void setHomeserver(const QUrl& baseUrl);

    /** Determine and set the homeserver from MXID */
    void resolveServer(const QString& mxid);

    void connectToServer(const QString& user, const QString& password,
                         const QString& initialDeviceName,
                         const QString& deviceId = {});
    void connectWithToken(const QString& userId, const QString& accessToken,
                          const QString& deviceId);
    /// Explicitly request capabilities from the server
    void reloadCapabilities();

    /// Find out if capabilites are still loading from the server
    bool loadingCapabilities() const;

    /** @deprecated Use stopSync() instead */
    void disconnectFromServer() { stopSync(); }
    void logout();

    void sync(int timeout = -1);
    void syncLoop(int timeout = -1);

    void stopSync();
    QString nextBatchToken() const;

    virtual MediaThumbnailJob*
    getThumbnail(const QString& mediaId, QSize requestedSize,
                 RunningPolicy policy = BackgroundRequest) const;
    MediaThumbnailJob*
    getThumbnail(const QUrl& url, QSize requestedSize,
                 RunningPolicy policy = BackgroundRequest) const;
    MediaThumbnailJob*
    getThumbnail(const QUrl& url, int requestedWidth, int requestedHeight,
                 RunningPolicy policy = BackgroundRequest) const;

    // QIODevice* should already be open
    UploadContentJob*
    uploadContent(QIODevice* contentSource, const QString& filename = {},
                  const QString& overrideContentType = {}) const;
    UploadContentJob* uploadFile(const QString& fileName,
                                 const QString& overrideContentType = {});
    GetContentJob* getContent(const QString& mediaId) const;
    GetContentJob* getContent(const QUrl& url) const;
    // If localFilename is empty, a temporary file will be created
    DownloadFileJob* downloadFile(const QUrl& url,
                                  const QString& localFilename = {}) const;

    /**
     * \brief Create a room (generic method)
     * This method allows to customize room entirely to your liking,
     * providing all the attributes the original CS API provides.
     */
    CreateRoomJob*
    createRoom(RoomVisibility visibility, const QString& alias,
               const QString& name, const QString& topic, QStringList invites,
               const QString& presetName = {}, const QString& roomVersion = {},
               bool isDirect = false,
               const QVector<CreateRoomJob::StateEvent>& initialState = {},
               const QVector<CreateRoomJob::Invite3pid>& invite3pids = {},
               const QJsonObject& creationContent = {});

    /** Get a direct chat with a single user
     * This method may return synchronously or asynchoronously depending
     * on whether a direct chat room with the respective person exists
     * already.
     *
     * \sa directChatAvailable
     */
    void requestDirectChat(const QString& userId);

    /** Get a direct chat with a single user
     * This method may return synchronously or asynchoronously depending
     * on whether a direct chat room with the respective person exists
     * already.
     *
     * \sa directChatAvailable
     */
    void requestDirectChat(User* u);

    /** Run an operation in a direct chat with the user
     * This method may return synchronously or asynchoronously depending
     * on whether a direct chat room with the respective person exists
     * already. Instead of emitting a signal it executes the passed
     * function object with the direct chat room as its parameter.
     */
    void doInDirectChat(const QString& userId,
                        const std::function<void(Room*)>& operation);

    /** Run an operation in a direct chat with the user
     * This method may return synchronously or asynchoronously depending
     * on whether a direct chat room with the respective person exists
     * already. Instead of emitting a signal it executes the passed
     * function object with the direct chat room as its parameter.
     */
    void doInDirectChat(User* u, const std::function<void(Room*)>& operation);

    /** Create a direct chat with a single user, optional name and topic
     * A room will always be created, unlike in requestDirectChat.
     * It is advised to use requestDirectChat as a default way of getting
     * one-on-one with a person, and only use createDirectChat when
     * a new creation is explicitly desired.
     */
    CreateRoomJob* createDirectChat(const QString& userId,
                                    const QString& topic = {},
                                    const QString& name = {});

    virtual JoinRoomJob* joinRoom(const QString& roomAlias,
                                  const QStringList& serverNames = {});

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
    ForgetRoomJob* forgetRoom(const QString& id);

    SendToDeviceJob* sendToDevices(const QString& eventType,
                                   const UsersToDevicesToEvents& eventsMap) const;

    /** \deprecated This method is experimental and may be removed any time */
    SendMessageJob* sendMessage(const QString& roomId,
                                const RoomEvent& event) const;

    /** \deprecated Do not use this directly, use Room::leaveRoom() instead */
    virtual LeaveRoomJob* leaveRoom(Room* room);

    // Old API that will be abolished any time soon. DO NOT USE.

    /** @deprecated Use callApi<PostReceiptJob>() or Room::postReceipt() instead
     */
    virtual PostReceiptJob* postReceipt(Room* room, RoomEvent* event) const;
signals:
    /**
     * @deprecated
     * This was a signal resulting from a successful resolveServer().
     * Since Connection now provides setHomeserver(), the HS URL
     * may change even without resolveServer() invocation. Use
     * homeserverChanged() instead of resolved(). You can also use
     * connectToServer and connectWithToken without the HS URL set in
     * advance (i.e. without calling resolveServer), as they now trigger
     * server name resolution from MXID if the server URL is not valid.
     */
    void resolved();
    void resolveError(QString error);

    void homeserverChanged(QUrl baseUrl);
    void capabilitiesLoaded();

    void connected();
    void reconnected(); //< \deprecated Use connected() instead
    void loggedOut();
    /** Login data or state have changed
     *
     * This is a common change signal for userId, deviceId and
     * accessToken - these properties normally only change at
     * a successful login and logout and are constant at other times.
     */
    void stateChanged();
    void loginError(QString message, QString details);

    /** A network request (job) failed
     *
     * @param request - the pointer to the failed job
     */
    void requestFailed(BaseJob* request);

    /** A network request (job) failed due to network problems
     *
     * This is _only_ emitted when the job will retry on its own;
     * once it gives up, requestFailed() will be emitted.
     *
     * @param message - message about the network problem
     * @param details - raw error details, if any available
     * @param retriesTaken - how many retries have already been taken
     * @param nextRetryInMilliseconds - when the job will retry again
     */
    void networkError(QString message, QString details, int retriesTaken,
                      int nextRetryInMilliseconds);

    void syncDone();
    void syncError(QString message, QString details);

    void newUser(User* user);

    /**
     * \group Signals emitted on room transitions
     *
     * Note: Rooms in Invite state are always stored separately from
     * rooms in Join/Leave state, because of special treatment of
     * invite_state in Matrix CS API (see The Spec on /sync for details).
     * Therefore, objects below are: r - room in Join/Leave state;
     * i - room in Invite state
     *
     * 1. none -> Invite: newRoom(r), invitedRoom(r,nullptr)
     * 2. none -> Join: newRoom(r), joinedRoom(r,nullptr)
     * 3. none -> Leave: newRoom(r), leftRoom(r,nullptr)
     * 4. Invite -> Join:
     *      newRoom(r), joinedRoom(r,i), aboutToDeleteRoom(i)
     * 4a. Leave and Invite -> Join:
     *      joinedRoom(r,i), aboutToDeleteRoom(i)
     * 5. Invite -> Leave:
     *      newRoom(r), leftRoom(r,i), aboutToDeleteRoom(i)
     * 5a. Leave and Invite -> Leave:
     *      leftRoom(r,i), aboutToDeleteRoom(i)
     * 6. Join -> Leave: leftRoom(r)
     * 7. Leave -> Invite: newRoom(i), invitedRoom(i,r)
     * 8. Leave -> Join: joinedRoom(r)
     * The following transitions are only possible via forgetRoom()
     * so far; if a room gets forgotten externally, sync won't tell
     * about it:
     * 9. any -> none: as any -> Leave, then aboutToDeleteRoom(r)
     */

    /** A new room object has been created */
    void newRoom(Room* room);

    /** A room invitation is seen for the first time
     *
     * If the same room is in Left state, it's passed in prev. Beware
     * that initial sync will trigger this signal for all rooms in
     * Invite state.
     */
    void invitedRoom(Room* room, Room* prev);

    /** A joined room is seen for the first time
     *
     * It's not the same as receiving a room in "join" section of sync
     * response (rooms will be there even after joining); it's also
     * not (exactly) the same as actual joining action of a user (all
     * rooms coming in initial sync will trigger this signal too). If
     * this room was in Invite state before, the respective object is
     * passed in prev (and it will be deleted shortly afterwards).
     */
    void joinedRoom(Room* room, Room* prev);

    /** A room has just been left
     *
     * If this room has been in Invite state (as in case of rejecting
     * an invitation), the respective object will be passed in prev
     * (and will be deleted shortly afterwards). Note that, similar
     * to invitedRoom and joinedRoom, this signal is triggered for all
     * Left rooms upon initial sync (not only those that were left
     * right before the sync).
     */
    void leftRoom(Room* room, Room* prev);

    /** The room object is about to be deleted */
    void aboutToDeleteRoom(Room* room);

    /** The room has just been created by createRoom or requestDirectChat
     *
     * This signal is not emitted in usual room state transitions,
     * only as an outcome of room creation operations invoked by
     * the client.
     * \note requestDirectChat doesn't necessarily create a new chat;
     *       use directChatAvailable signal if you just need to obtain
     *       a direct chat room.
     */
    void createdRoom(Room* room);

    /** The first sync for the room has been completed
     *
     * This signal is emitted after the room has been synced the first
     * time. This is the right signal to connect to if you need to
     * access the room state (name, aliases, members); state transition
     * signals (newRoom, joinedRoom etc.) come earlier, when the room
     * has just been created.
     */
    void loadedRoomState(Room* room);

    /** Account data (except direct chats) have changed */
    void accountDataChanged(QString type);

    /** The direct chat room is ready for using
     * This signal is emitted upon any successful outcome from
     * requestDirectChat.
     */
    void directChatAvailable(Room* directChat);

    /** The list of direct chats has changed
     * This signal is emitted every time when the mapping of users
     * to direct chat rooms is changed (because of either local updates
     * or a different list arrived from the server).
     */
    void directChatsListChanged(DirectChatsMap additions,
                                DirectChatsMap removals);

    void ignoredUsersListChanged(IgnoredUsersList additions,
                                 IgnoredUsersList removals);

    void cacheStateChanged();
    void lazyLoadingChanged();
    void turnServersChanged(const QJsonObject& servers);

protected:
    /**
     * @brief Access the underlying ConnectionData class
     */
    const ConnectionData* connectionData() const;

    /** Get a Room object for the given id in the given state
     *
     * Use this method when you need a Room object in the local list
     * of rooms, with the given state. Note that this does not interact
     * with the server; in particular, does not automatically create
     * rooms on the server. This call performs necessary join state
     * transitions; e.g., if it finds a room in Invite but
     * `joinState == JoinState::Join` then the Invite room object
     * will be deleted and a new room object with Join state created.
     * In contrast, switching between Join and Leave happens within
     * the same object.
     * \param roomId room id (not alias!)
     * \param joinState desired (target) join state of the room; if
     * omitted, any state will be found and return unchanged, or a
     * new Join room created.
     * @return a pointer to a Room object with the specified id and the
     * specified state; nullptr if roomId is empty or if roomFactory()
     * failed to create a Room object.
     */
    Room* provideRoom(const QString& roomId,
                      Omittable<JoinState> joinState = none);

    /**
     * Completes loading sync data.
     */
    void onSyncSuccess(SyncData&& data, bool fromCache = false);

protected slots:
    void syncLoopIteration();

private:
    class Private;
    QScopedPointer<Private> d;

    /**
     * A single entry for functions that need to check whether the
     * homeserver is valid before running. May either execute connectFn
     * synchronously or asynchronously (if tryResolve is true and
     * a DNS lookup is initiated); in case of errors, emits resolveError
     * if the homeserver URL is not valid and cannot be resolved from
     * userId.
     *
     * @param userId - fully-qualified MXID to resolve HS from
     * @param connectFn - a function to execute once the HS URL is good
     */
    void checkAndConnect(const QString& userId, std::function<void()> connectFn);
    void doConnectToServer(const QString& user, const QString& password,
                           const QString& initialDeviceName,
                           const QString& deviceId = {});

    static room_factory_t _roomFactory;
    static user_factory_t _userFactory;
};
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::Connection*)

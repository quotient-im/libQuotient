// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ssosession.h"
#include "qt_connection_util.h"
#include "quotient_common.h"

#include "csapi/login.h"
#include "csapi/create_room.h"

#include "events/accountdataevents.h"

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QUrl>

#include <functional>

Q_DECLARE_METATYPE(Quotient::GetLoginFlowsJob::LoginFlow)

namespace Quotient {

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

class QOlmAccount;

using LoginFlow = GetLoginFlowsJob::LoginFlow;

/// Predefined login flows
struct LoginFlows {
    static inline const LoginFlow Password { "m.login.password" };
    static inline const LoginFlow SSO { "m.login.sso" };
    static inline const LoginFlow Token { "m.login.token" };
};

// To simplify comparisons of LoginFlows

inline bool operator==(const LoginFlow& lhs, const LoginFlow& rhs)
{
    return lhs.type == rhs.type;
}

inline bool operator!=(const LoginFlow& lhs, const LoginFlow& rhs)
{
    return !(lhs == rhs);
}

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

// Room ids, rather than room pointers, are used in the direct chat
// map types because the library keeps Invite rooms separate from
// rooms in Join and Leave state; and direct chats in account data
// are stored with no regard to their state.
using DirectChatsMap = QMultiHash<const User*, QString>;
using DirectChatUsersMap = QMultiHash<QString, User*>;
using IgnoredUsersList = IgnoredUsersEvent::content_type;

class Connection : public QObject {
    Q_OBJECT

    Q_PROPERTY(User* localUser READ user NOTIFY stateChanged)
    Q_PROPERTY(QString localUserId READ userId NOTIFY stateChanged)
    Q_PROPERTY(QString domain READ domain NOTIFY stateChanged STORED false)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY stateChanged)
    Q_PROPERTY(QByteArray accessToken READ accessToken NOTIFY stateChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY stateChanged STORED false)
    Q_PROPERTY(QString defaultRoomVersion READ defaultRoomVersion NOTIFY capabilitiesLoaded)
    Q_PROPERTY(QUrl homeserver READ homeserver WRITE setHomeserver NOTIFY homeserverChanged)
    Q_PROPERTY(QVector<GetLoginFlowsJob::LoginFlow> loginFlows READ loginFlows NOTIFY loginFlowsChanged)
    Q_PROPERTY(bool isUsable READ isUsable NOTIFY loginFlowsChanged STORED false)
    Q_PROPERTY(bool supportsSso READ supportsSso NOTIFY loginFlowsChanged STORED false)
    Q_PROPERTY(bool supportsPasswordAuth READ supportsPasswordAuth NOTIFY loginFlowsChanged STORED false)
    Q_PROPERTY(bool cacheState READ cacheState WRITE setCacheState NOTIFY cacheStateChanged)
    Q_PROPERTY(bool lazyLoading READ lazyLoading WRITE setLazyLoading NOTIFY lazyLoadingChanged)

public:
    using UsersToDevicesToEvents =
        UnorderedMap<QString, UnorderedMap<QString, const Event&>>;

    enum RoomVisibility {
        PublishRoom,
        UnpublishRoom
    }; // FIXME: Should go inside CreateRoomJob

    explicit Connection(QObject* parent = nullptr);
    explicit Connection(const QUrl& server, QObject* parent = nullptr);
    ~Connection() override;

    /// Get all rooms known within this Connection
    /*!
     * This includes Invite, Join and Leave rooms, in no particular order.
     * \note Leave rooms will only show up in the list if they have been left
     *       in the same running session. The library doesn't cache left rooms
     *       between runs and it doesn't retrieve the full list of left rooms
     *       from the server.
     * \sa rooms, room, roomsWithTag
     */
    Q_INVOKABLE QVector<Quotient::Room*> allRooms() const;

    /// Get rooms that have either of the given join state(s)
    /*!
     * This method returns, in no particular order, rooms which join state
     * matches the mask passed in \p joinStates.
     * \note Similar to allRooms(), this won't retrieve the full list of
     *       Leave rooms from the server.
     * \sa allRooms, room, roomsWithTag
     */
    Q_INVOKABLE QVector<Quotient::Room*>
    rooms(Quotient::JoinStates joinStates) const;

    /// Get the total number of rooms in the given join state(s)
    Q_INVOKABLE int roomsCount(Quotient::JoinStates joinStates) const;

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

    /*! \brief Mark the room as a direct chat with the user
     *
     * This function marks \p room as a direct chat with \p user.
     * Emits the signal synchronously, without waiting to complete
     * synchronisation with the server.
     *
     * \sa directChatsListChanged
     */
    void addToDirectChats(const Room* room, User* user);

    /*! \brief Unmark the room from direct chats
     *
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
    Q_INVOKABLE bool isIgnored(const Quotient::User* user) const;

    /** Get the whole list of ignored users */
    Q_INVOKABLE Quotient::IgnoredUsersList ignoredUsers() const;

    /** Add the user to the ignore list
     * The change signal is emitted synchronously, without waiting
     * to complete synchronisation with the server.
     *
     * \sa ignoredUsersListChanged
     */
    Q_INVOKABLE void addToIgnoredUsers(const Quotient::User* user);

    /** Remove the user from the ignore list */
    /** Similar to adding, the change signal is emitted synchronously.
     *
     * \sa ignoredUsersListChanged
     */
    Q_INVOKABLE void removeFromIgnoredUsers(const Quotient::User* user);

    /** Get the full list of users known to this account */
    QMap<QString, User*> users() const;

    /** Get the base URL of the homeserver to connect to */
    QUrl homeserver() const;
    /** Get the domain name used for ids/aliases on the server */
    QString domain() const;
    /** Check if the homeserver is known to be reachable and working */
    bool isUsable() const;
    /** Get the list of supported login flows */
    QVector<GetLoginFlowsJob::LoginFlow> loginFlows() const;
    /** Check whether the current homeserver supports password auth */
    bool supportsPasswordAuth() const;
    /** Check whether the current homeserver supports SSO */
    bool supportsSso() const;
    /** Find a room by its id and a mask of applicable states */
    Q_INVOKABLE Quotient::Room*
    room(const QString& roomId,
         Quotient::JoinStates states = JoinState::Invite | JoinState::Join) const;
    /** Find a room by its alias and a mask of applicable states */
    Q_INVOKABLE Quotient::Room*
    roomByAlias(const QString& roomAlias,
                Quotient::JoinStates states = JoinState::Invite
                                              | JoinState::Join) const;
    /** Update the internal map of room aliases to IDs */
    /// This is used to maintain the internal index of room aliases.
    /// It does NOT change aliases on the server,
    /// \sa Room::setLocalAliases
    void updateRoomAliases(const QString& roomId,
                           const QStringList& previousRoomAliases,
                           const QStringList& roomAliases);
    Q_INVOKABLE Quotient::Room* invitation(const QString& roomId) const;
    Q_INVOKABLE Quotient::User* user(const QString& uId);
    const User* user() const;
    User* user();
    QString userId() const;
    QString deviceId() const;
    QByteArray accessToken() const;
    bool isLoggedIn() const;
#ifdef Quotient_E2EE_ENABLED
    QOlmAccount* olmAccount() const;
#endif // Quotient_E2EE_ENABLED
    Q_INVOKABLE Quotient::SyncJob* syncJob() const;
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

    /// Indicate if the user can change its password from the client.
    /// This is often not the case when SSO is enabled.
    /// \sa loadingCapabilities
    bool canChangePassword() const;

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
    Q_INVOKABLE BaseJob* run(BaseJob* job,
                         RunningPolicy runningPolicy = ForegroundRequest);

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
    JobT* callApi(RunningPolicy runningPolicy, JobArgTs&&... jobArgs)
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
    JobT* callApi(JobArgTs&&... jobArgs)
    {
        return callApi<JobT>(ForegroundRequest,
                             std::forward<JobArgTs>(jobArgs)...);
    }

    /*! Get a request URL for a job with specified type and arguments
     *
     * This calls JobT::makeRequestUrl() prepending the connection's homeserver
     * to the list of arguments.
     */
    template <typename JobT, typename... JobArgTs>
    QUrl getUrlForApi(JobArgTs&&... jobArgs) const
    {
        return JobT::makeRequestUrl(homeserver(),
                                    std::forward<JobArgTs>(jobArgs)...);
    }

    Q_INVOKABLE SsoSession* prepareForSso(const QString& initialDeviceName,
                                          const QString& deviceId = {});

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

    /// Ignore ssl errors (usefull for automated testing with local synapse
    /// instance).
    /// \internal
    void ignoreSslErrors(bool ignore);

public Q_SLOTS:
    /// \brief Set the homeserver base URL and retrieve its login flows
    ///
    /// \sa LoginFlowsJob, loginFlows, loginFlowsChanged, homeserverChanged
    void setHomeserver(const QUrl& baseUrl);

    /// \brief Determine and set the homeserver from MXID
    ///
    /// This attempts to resolve the homeserver by requesting
    /// .well-known/matrix/client record from the server taken from the MXID
    /// serverpart. If there is no record found, the serverpart itself is
    /// attempted as the homeserver base URL; if the record is there but
    /// is malformed (e.g., the homeserver base URL cannot be found in it)
    /// resolveError() is emitted and further processing stops. Otherwise,
    /// setHomeserver is called, preparing the Connection object for the login
    /// attempt.
    /// \param mxid user Matrix ID, such as @someone:example.org
    /// \sa setHomeserver, homeserverChanged, loginFlowsChanged, resolveError
    void resolveServer(const QString& mxid);

    /** \brief Log in using a username and password pair
     *
     * Before logging in, this method checks if the homeserver is valid and
     * supports the password login flow. If the homeserver is invalid but
     * a full user MXID is provided, this method calls resolveServer() using
     * this MXID.
     *
     * \sa resolveServer, resolveError, loginError
     */
    void loginWithPassword(const QString& userId, const QString& password,
                           const QString& initialDeviceName,
                           const QString& deviceId = {});
    /** \brief Log in using a login token
     *
     * One usual case for this method is the final stage of logging in via SSO.
     * Unlike loginWithPassword() and assumeIdentity(), this method cannot
     * resolve the server from the user name because the full user MXID is
     * encoded in the login token. Callers should ensure the homeserver
     * sanity in advance.
     */
    void loginWithToken(const QByteArray& loginToken,
                        const QString& initialDeviceName,
                        const QString& deviceId = {});
    /** \brief Use an existing access token to connect to the homeserver
     *
     * Similar to loginWithPassword(), this method checks that the homeserver
     * URL is valid and tries to resolve it from the MXID in case it is not.
     */
    void assumeIdentity(const QString& mxId, const QString& accessToken,
                        const QString& deviceId);
    /// Explicitly request capabilities from the server
    void reloadCapabilities();

    /// Find out if capabilites are still loading from the server
    bool loadingCapabilities() const;

    void logout();

    void sync(int timeout = -1);
    void syncLoop(int timeout = 30000);

    void stopSync();
    QString nextBatchToken() const;

    Q_INVOKABLE QUrl makeMediaUrl(QUrl mxcUrl) const;

    virtual MediaThumbnailJob*
    getThumbnail(const QString& mediaId, QSize requestedSize,
                 RunningPolicy policy = BackgroundRequest);
    MediaThumbnailJob* getThumbnail(const QUrl& url, QSize requestedSize,
                                    RunningPolicy policy = BackgroundRequest);
    MediaThumbnailJob* getThumbnail(const QUrl& url, int requestedWidth,
                                    int requestedHeight,
                                    RunningPolicy policy = BackgroundRequest);

    // QIODevice* should already be open
    UploadContentJob* uploadContent(QIODevice* contentSource,
                                    const QString& filename = {},
                                    const QString& overrideContentType = {});
    UploadContentJob* uploadFile(const QString& fileName,
                                 const QString& overrideContentType = {});
    GetContentJob* getContent(const QString& mediaId);
    GetContentJob* getContent(const QUrl& url);
    // If localFilename is empty, a temporary file will be created
    DownloadFileJob* downloadFile(const QUrl& url,
                                  const QString& localFilename = {});

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
                                   const UsersToDevicesToEvents& eventsMap);

    /** \deprecated This method is experimental and may be removed any time */
    SendMessageJob* sendMessage(const QString& roomId, const RoomEvent& event);

    /** \deprecated Do not use this directly, use Room::leaveRoom() instead */
    virtual LeaveRoomJob* leaveRoom(Room* room);

Q_SIGNALS:
    /// \brief Initial server resolution has failed
    ///
    /// This signal is emitted when resolveServer() did not manage to resolve
    /// the homeserver using its .well-known/client record or otherwise.
    /// \sa resolveServer
    void resolveError(QString error);

    void homeserverChanged(QUrl baseUrl);
    void loginFlowsChanged();
    void capabilitiesLoaded();

    void connected();
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
    void requestFailed(Quotient::BaseJob* request);

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

    void newUser(Quotient::User* user);

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
    void newRoom(Quotient::Room* room);

    /** A room invitation is seen for the first time
     *
     * If the same room is in Left state, it's passed in prev. Beware
     * that initial sync will trigger this signal for all rooms in
     * Invite state.
     */
    void invitedRoom(Quotient::Room* room, Quotient::Room* prev);

    /** A joined room is seen for the first time
     *
     * It's not the same as receiving a room in "join" section of sync
     * response (rooms will be there even after joining); it's also
     * not (exactly) the same as actual joining action of a user (all
     * rooms coming in initial sync will trigger this signal too). If
     * this room was in Invite state before, the respective object is
     * passed in prev (and it will be deleted shortly afterwards).
     */
    void joinedRoom(Quotient::Room* room, Quotient::Room* prev);

    /** A room has just been left
     *
     * If this room has been in Invite state (as in case of rejecting
     * an invitation), the respective object will be passed in prev
     * (and will be deleted shortly afterwards). Note that, similar
     * to invitedRoom and joinedRoom, this signal is triggered for all
     * Left rooms upon initial sync (not only those that were left
     * right before the sync).
     */
    void leftRoom(Quotient::Room* room, Quotient::Room* prev);

    /** The room object is about to be deleted */
    void aboutToDeleteRoom(Quotient::Room* room);

    /** The room has just been created by createRoom or requestDirectChat
     *
     * This signal is not emitted in usual room state transitions,
     * only as an outcome of room creation operations invoked by
     * the client.
     * \note requestDirectChat doesn't necessarily create a new chat;
     *       use directChatAvailable signal if you just need to obtain
     *       a direct chat room.
     */
    void createdRoom(Quotient::Room* room);

    /** The first sync for the room has been completed
     *
     * This signal is emitted after the room has been synced the first
     * time. This is the right signal to connect to if you need to
     * access the room state (name, aliases, members); state transition
     * signals (newRoom, joinedRoom etc.) come earlier, when the room
     * has just been created.
     */
    void loadedRoomState(Quotient::Room* room);

    /** Account data (except direct chats) have changed */
    void accountDataChanged(QString type);

    /** The direct chat room is ready for using
     * This signal is emitted upon any successful outcome from
     * requestDirectChat.
     */
    void directChatAvailable(Quotient::Room* directChat);

    /** The list of direct chats has changed
     * This signal is emitted every time when the mapping of users
     * to direct chat rooms is changed (because of either local updates
     * or a different list arrived from the server).
     */
    void directChatsListChanged(Quotient::DirectChatsMap additions,
                                Quotient::DirectChatsMap removals);

    void ignoredUsersListChanged(Quotient::IgnoredUsersList additions,
                                 Quotient::IgnoredUsersList removals);

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

protected Q_SLOTS:
    void syncLoopIteration();

private:
    class Private;
    QScopedPointer<Private> d;

    static room_factory_t _roomFactory;
    static user_factory_t _userFactory;
};
} // namespace Quotient
Q_DECLARE_METATYPE(Quotient::DirectChatsMap)
Q_DECLARE_METATYPE(Quotient::IgnoredUsersList)

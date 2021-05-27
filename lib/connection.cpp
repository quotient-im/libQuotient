// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2019 Ville Ranki <ville.ranki@iki.fi>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "connection.h"

#include "connectiondata.h"
#ifdef Quotient_E2EE_ENABLED
#    include "encryptionmanager.h"
#endif // Quotient_E2EE_ENABLED
#include "room.h"
#include "settings.h"
#include "user.h"

#include "csapi/account-data.h"
#include "csapi/capabilities.h"
#include "csapi/joining.h"
#include "csapi/leaving.h"
#include "csapi/logout.h"
#include "csapi/receipts.h"
#include "csapi/room_send.h"
#include "csapi/to_device.h"
#include "csapi/versions.h"
#include "csapi/voip.h"
#include "csapi/wellknown.h"
#include "csapi/whoami.h"

#include "events/directchatevent.h"
#include "events/eventloader.h"
#include "jobs/downloadfilejob.h"
#include "jobs/mediathumbnailjob.h"
#include "jobs/syncjob.h"

#ifdef Quotient_E2EE_ENABLED
#    include "crypto/qolmaccount.h"
#endif // Quotient_E2EE_ENABLED

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
#    include <QtCore/QCborValue>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtNetwork/QDnsLookup>

using namespace Quotient;

// This is very much Qt-specific; STL iterators don't have key() and value()
template <typename HashT, typename Pred>
HashT erase_if(HashT& hashMap, Pred pred)
{
    HashT removals;
    for (auto it = hashMap.begin(); it != hashMap.end();) {
        if (pred(it)) {
            removals.insert(it.key(), it.value());
            it = hashMap.erase(it);
        } else
            ++it;
    }
    return removals;
}

class Connection::Private {
public:
    explicit Private(std::unique_ptr<ConnectionData>&& connection)
        : data(move(connection))
    {}
    Q_DISABLE_COPY(Private)
    DISABLE_MOVE(Private)

    Connection* q = nullptr;
    std::unique_ptr<ConnectionData> data;
    // A complex key below is a pair of room name and whether its
    // state is Invited. The spec mandates to keep Invited room state
    // separately; specifically, we should keep objects for Invite and
    // Leave state of the same room if the two happen to co-exist.
    QHash<QPair<QString, bool>, Room*> roomMap;
    /// Mapping from serverparts to alias/room id mappings,
    /// as of the last sync
    QHash<QString, QString> roomAliasMap;
    QVector<QString> roomIdsToForget;
    QVector<Room*> firstTimeRooms;
    QVector<QString> pendingStateRoomIds;
    QMap<QString, User*> userMap;
    DirectChatsMap directChats;
    DirectChatUsersMap directChatUsers;
    // The below two variables track local changes between sync completions.
    // See https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
    DirectChatsMap dcLocalAdditions;
    DirectChatsMap dcLocalRemovals;
    UnorderedMap<QString, EventPtr> accountData;
    QMetaObject::Connection syncLoopConnection {};
    int syncTimeout = -1;

    GetCapabilitiesJob* capabilitiesJob = nullptr;
    GetCapabilitiesJob::Capabilities capabilities;

    QVector<GetLoginFlowsJob::LoginFlow> loginFlows;

#ifdef Quotient_E2EE_ENABLED
    std::unique_ptr<QOlmAccount> olmAccount;
    bool isUploadingKeys = false;
    QScopedPointer<EncryptionManager> encryptionManager;
#endif // Quotient_E2EE_ENABLED

    QPointer<GetWellknownJob> resolverJob = nullptr;
    QPointer<GetLoginFlowsJob> loginFlowsJob = nullptr;

    SyncJob* syncJob = nullptr;
    QPointer<LogoutJob> logoutJob = nullptr;

    bool cacheState = true;
    bool cacheToBinary =
        SettingsGroup("libQuotient").get("cache_type",
                 SettingsGroup("libQMatrixClient").get<QString>("cache_type"))
        != "json";
    bool lazyLoading = false;

    /** \brief Check the homeserver and resolve it if needed, before connecting
     *
     * A single entry for functions that need to check whether the homeserver
     * is valid before running. May execute connectFn either synchronously
     * or asynchronously. In case of errors, emits resolveError() if
     * the homeserver URL is not valid and cannot be resolved from userId, or
     * the homeserver doesn't support the requested login flow.
     *
     * \param userId    fully-qualified MXID to resolve HS from
     * \param connectFn a function to execute once the HS URL is good
     * \param flow      optionally, a login flow that should be supported for
     *                  connectFn to work; `none`, if there's no login flow
     *                  requirements
     * \sa resolveServer, resolveError
     */
    void checkAndConnect(const QString &userId,
                         const std::function<void ()> &connectFn,
                         const std::optional<LoginFlow> &flow = none);
    template <typename... LoginArgTs>
    void loginToServer(LoginArgTs&&... loginArgs);
    void completeSetup(const QString &mxId);
    void removeRoom(const QString& roomId);

    void consumeRoomData(SyncDataList&& roomDataList, bool fromCache);
    void consumeAccountData(Events&& accountDataEvents);
    void consumePresenceData(Events&& presenceData);
    void consumeToDeviceEvents(Events&& toDeviceEvents);

    template <typename EventT>
    EventT* unpackAccountData() const
    {
        const auto& eventIt = accountData.find(EventT::matrixTypeId());
        return eventIt == accountData.end()
                   ? nullptr
                   : weakPtrCast<EventT>(eventIt->second);
    }

    void packAndSendAccountData(EventPtr&& event)
    {
        const auto eventType = event->matrixType();
        q->callApi<SetAccountDataJob>(data->userId(), eventType,
                                      event->contentJson());
        accountData[eventType] = std::move(event);
        emit q->accountDataChanged(eventType);
    }

    template <typename EventT, typename ContentT>
    void packAndSendAccountData(ContentT&& content)
    {
        packAndSendAccountData(
            makeEvent<EventT>(std::forward<ContentT>(content)));
    }
    QString topLevelStatePath() const
    {
        return q->stateCacheDir().filePath("state.json");
    }

    EventPtr sessionDecryptMessage(const EncryptedEvent& encryptedEvent)
    {
        qCWarning(E2EE) << "End-to-end encryption (E2EE) support is turned off.";
        return {};
#ifndef Quotient_E2EE_ENABLED
        qCWarning(E2EE) << "End-to-end encryption (E2EE) support is turned off.";
        return {};
#else // Quotient_E2EE_ENABLED
        if (encryptedEvent.algorithm() != OlmV1Curve25519AesSha2AlgoKey)
            return {};

        const auto identityKey =
            encryptionManager->account()->identityKeys().curve25519;
        const auto personalCipherObject =
            encryptedEvent.ciphertext(identityKey);
        if (personalCipherObject.isEmpty()) {
            qCDebug(E2EE) << "Encrypted event is not for the current device";
            return {};
        }
        const auto decrypted = encryptionManager->sessionDecryptMessage(
            personalCipherObject, encryptedEvent.senderKey().toLatin1());
        if (decrypted.isEmpty()) {
            qCDebug(E2EE) << "Problem with new session from senderKey:"
                          << encryptedEvent.senderKey()
                          << encryptionManager->account()->oneTimeKeys().keys;
            return {};
        }

        auto&& decryptedEvent =
            fromJson<EventPtr>(QJsonDocument::fromJson(decrypted.toUtf8()));

        if (auto sender = decryptedEvent->fullJson()[SenderKeyL].toString();
                sender != encryptedEvent.senderId()) {
            qCWarning(E2EE) << "Found user" << sender
                          << "instead of sender" << encryptedEvent.senderId()
                          << "in Olm plaintext";
            return {};
        }

        // TODO: keys to constants
        const auto decryptedEventObject = decryptedEvent->fullJson();
        const auto recipient =
            decryptedEventObject.value("recipient"_ls).toString();
        if (recipient != data->userId()) {
            qCDebug(E2EE) << "Found user" << recipient << "instead of us"
                          << data->userId() << "in Olm plaintext";
            return {};
        }
        const auto ourKey =
            decryptedEventObject.value("recipient_keys"_ls).toObject()
                .value(Ed25519Key).toString();
        if (ourKey
            != QString::fromUtf8(
                encryptionManager->account()->identityKeys().ed25519)) {
            qCDebug(E2EE) << "Found key" << ourKey
                          << "instead of ours own ed25519 key"
                          << encryptionManager->account()->identityKeys().ed25519
                          << "in Olm plaintext";
            return {};
        }

        return std::move(decryptedEvent);
#endif // Quotient_E2EE_ENABLED
    }
};

Connection::Connection(const QUrl& server, QObject* parent)
    : QObject(parent), d(new Private(std::make_unique<ConnectionData>(server)))
{
    d->q = this; // All d initialization should occur before this line
}

Connection::Connection(QObject* parent) : Connection({}, parent) {}

Connection::~Connection()
{
    qCDebug(MAIN) << "deconstructing connection object for" << userId();
    stopSync();
}

void Connection::resolveServer(const QString& mxid)
{
    if (isJobPending(d->resolverJob))
        d->resolverJob->abandon();

    auto maybeBaseUrl = QUrl::fromUserInput(serverPart(mxid));
    maybeBaseUrl.setScheme("https"); // Instead of the Qt-default "http"
    if (maybeBaseUrl.isEmpty() || !maybeBaseUrl.isValid()) {
        emit resolveError(tr("%1 is not a valid homeserver address")
                              .arg(maybeBaseUrl.toString()));
        return;
    }

    qCDebug(MAIN) << "Finding the server" << maybeBaseUrl.host();

    const auto& oldBaseUrl = d->data->baseUrl();
    d->data->setBaseUrl(maybeBaseUrl); // Temporarily set it for this one call
    d->resolverJob = callApi<GetWellknownJob>();
    // Connect to finished() to make sure baseUrl is restored in any case
    connect(d->resolverJob, &BaseJob::finished, this, [this, maybeBaseUrl, oldBaseUrl] {
        // Revert baseUrl so that setHomeserver() below triggers signals
        // in case the base URL actually changed
        d->data->setBaseUrl(oldBaseUrl);
        if (d->resolverJob->error() == BaseJob::Abandoned)
            return;

        if (d->resolverJob->error() != BaseJob::NotFoundError) {
            if (!d->resolverJob->status().good()) {
                qCWarning(MAIN)
                    << "Fetching .well-known file failed, FAIL_PROMPT";
                emit resolveError(tr("Failed resolving the homeserver"));
                return;
            }
            QUrl baseUrl { d->resolverJob->data().homeserver.baseUrl };
            if (baseUrl.isEmpty()) {
                qCWarning(MAIN) << "base_url not provided, FAIL_PROMPT";
                emit resolveError(
                    tr("The homeserver base URL is not provided"));
                return;
            }
            if (!baseUrl.isValid()) {
                qCWarning(MAIN) << "base_url invalid, FAIL_ERROR";
                emit resolveError(tr("The homeserver base URL is invalid"));
                return;
            }
            qCInfo(MAIN) << ".well-known URL for" << maybeBaseUrl.host() << "is"
                         << baseUrl.toString();
            setHomeserver(baseUrl);
        } else {
            qCInfo(MAIN) << "No .well-known file, using" << maybeBaseUrl
                         << "for base URL";
            setHomeserver(maybeBaseUrl);
        }
        Q_ASSERT(d->loginFlowsJob != nullptr); // Ensured by setHomeserver()
        connect(d->loginFlowsJob, &BaseJob::success, this,
                &Connection::resolved);
        connect(d->loginFlowsJob, &BaseJob::failure, this, [this] {
            qCWarning(MAIN) << "Homeserver base URL sanity check failed";
            emit resolveError(tr("The homeserver doesn't seem to be working"));
        });
    });
}

inline UserIdentifier makeUserIdentifier(const QString& id)
{
    return { QStringLiteral("m.id.user"), { { QStringLiteral("user"), id } } };
}

inline UserIdentifier make3rdPartyIdentifier(const QString& medium,
                                             const QString& address)
{
    return { QStringLiteral("m.id.thirdparty"),
             { { QStringLiteral("medium"), medium },
               { QStringLiteral("address"), address } } };
}

void Connection::loginWithPassword(const QString& userId,
                                   const QString& password,
                                   const QString& initialDeviceName,
                                   const QString& deviceId)
{
    d->checkAndConnect(userId, [=] {
        d->loginToServer(LoginFlows::Password.type, makeUserIdentifier(userId),
                         password, /*token*/ "", deviceId, initialDeviceName);
    }, LoginFlows::Password);
}

SsoSession* Connection::prepareForSso(const QString& initialDeviceName,
                                      const QString& deviceId)
{
    return new SsoSession(this, initialDeviceName, deviceId);
}

void Connection::loginWithToken(const QByteArray& loginToken,
                                const QString& initialDeviceName,
                                const QString& deviceId)
{
    Q_ASSERT(d->data->baseUrl().isValid() && d->loginFlows.contains(LoginFlows::Token));
    d->loginToServer(LoginFlows::Token.type,
                     none /*user is encoded in loginToken*/, "" /*password*/,
                     loginToken, deviceId, initialDeviceName);
}

void Connection::assumeIdentity(const QString& mxId, const QString& accessToken,
                                const QString& deviceId)
{
    d->checkAndConnect(mxId, [this, mxId, accessToken, deviceId] {
        d->data->setToken(accessToken.toLatin1());
        d->data->setDeviceId(deviceId); // Can't we deduce this from access_token?
        auto* job = callApi<GetTokenOwnerJob>();
        connect(job, &BaseJob::success, this, [this, job, mxId] {
            if (mxId != job->userId())
                qCWarning(MAIN).nospace()
                    << "The access_token owner (" << job->userId()
                    << ") is different from passed MXID (" << mxId << ")!";
            d->completeSetup(job->userId());
        });
        connect(job, &BaseJob::failure, this, [this, job] {
            emit loginError(job->errorString(), job->rawDataSample());
        });
    });
}

void Connection::reloadCapabilities()
{
    d->capabilitiesJob = callApi<GetCapabilitiesJob>(BackgroundRequest);
    connect(d->capabilitiesJob, &BaseJob::success, this, [this] {
        d->capabilities = d->capabilitiesJob->capabilities();

        if (d->capabilities.roomVersions) {
            qCDebug(MAIN) << "Room versions:" << defaultRoomVersion()
                          << "is default, full list:" << availableRoomVersions();
            emit capabilitiesLoaded();
            for (auto* r: std::as_const(d->roomMap))
                r->checkVersion();
        } else
            qCWarning(MAIN)
                << "The server returned an empty set of supported versions;"
                   " disabling version upgrade recommendations to reduce noise";
    });
    connect(d->capabilitiesJob, &BaseJob::failure, this, [this] {
        if (d->capabilitiesJob->error() == BaseJob::IncorrectRequestError)
            qCDebug(MAIN) << "Server doesn't support /capabilities;"
                             " version upgrade recommendations won't be issued";
    });
}

bool Connection::loadingCapabilities() const
{
    // (Ab)use the fact that room versions cannot be omitted after
    // the capabilities have been loaded (see reloadCapabilities() above).
    return !d->capabilities.roomVersions;
}

template <typename... LoginArgTs>
void Connection::Private::loginToServer(LoginArgTs&&... loginArgs)
{
    auto loginJob =
            q->callApi<LoginJob>(std::forward<LoginArgTs>(loginArgs)...);
    connect(loginJob, &BaseJob::success, q, [this, loginJob] {
        data->setToken(loginJob->accessToken().toLatin1());
        data->setDeviceId(loginJob->deviceId());
        completeSetup(loginJob->userId());
#ifndef Quotient_E2EE_ENABLED
        qCWarning(E2EE) << "End-to-end encryption (E2EE) support is turned off.";
#else // Quotient_E2EE_ENABLED
        //encryptionManager->uploadIdentityKeys(q);
        //encryptionManager->uploadOneTimeKeys(q);
#endif // Quotient_E2EE_ENABLED
    });
    connect(loginJob, &BaseJob::failure, q, [this, loginJob] {
        emit q->loginError(loginJob->errorString(), loginJob->rawDataSample());
    });
}

void Connection::Private::completeSetup(const QString& mxId)
{
    data->setUserId(mxId);
    q->user(); // Creates a User object for the local user
    q->setObjectName(data->userId() % '/' % data->deviceId());
    qCDebug(MAIN) << "Using server" << data->baseUrl().toDisplayString()
                  << "by user" << data->userId()
                  << "from device" << data->deviceId();
#ifndef Quotient_E2EE_ENABLED
    qCWarning(E2EE) << "End-to-end encryption (E2EE) support is turned off.";
#else // Quotient_E2EE_ENABLED
    AccountSettings accountSettings(data->userId());

    // init olmAccount
    olmAccount = std::make_unique<QOlmAccount>(data->userId(), data->deviceId());

    if (accountSettings.encryptionAccountPickle().isEmpty()) {
        // create new account and save unpickle data
        olmAccount->createNewAccount();
        accountSettings.setEncryptionAccountPickle(std::get<QByteArray>(olmAccount->pickle(Unencrypted{})));
        // TODO handle pickle errors
        auto job = q->callApi<UploadKeysJob>(olmAccount->deviceKeys());
        connect(job, &BaseJob::failure, q, [=]{
            qCWarning(E2EE) << "Failed to upload device keys:" << job->errorString();
        });
    } else {
        // account already existing
        auto pickle = accountSettings.encryptionAccountPickle();
        olmAccount->unpickle(pickle, Unencrypted{});
    }
#endif // Quotient_E2EE_ENABLED
    emit q->stateChanged();
    emit q->connected();
    q->reloadCapabilities();
}

void Connection::Private::checkAndConnect(const QString& userId,
                                          const std::function<void()>& connectFn,
                                          const std::optional<LoginFlow>& flow)
{
    if (data->baseUrl().isValid() && (!flow || loginFlows.contains(*flow))) {
        connectFn();
        return;
    }
    // Not good to go, try to ascertain the homeserver URL and flows
    if (userId.startsWith('@') && userId.indexOf(':') != -1) {
        q->resolveServer(userId);
        if (flow)
            connectSingleShot(q, &Connection::loginFlowsChanged, q,
                [this, flow, connectFn] {
                    if (loginFlows.contains(*flow))
                        connectFn();
                    else
                        emit q->loginError(
                            tr("The homeserver at %1 does not support"
                               " the login flow '%2'")
                            .arg(data->baseUrl().toDisplayString()),
                                 flow->type);
                });
        else
            connectSingleShot(q, &Connection::homeserverChanged, q, connectFn);
    } else
        emit q->resolveError(tr("Please provide the fully-qualified user id"
                                " (such as @user:example.org) so that the"
                                " homeserver could be resolved; the current"
                                " homeserver URL(%1) is not good")
                             .arg(data->baseUrl().toDisplayString()));
}

void Connection::logout()
{
    // If there's an ongoing sync job, stop it (this also suspends sync loop)
    const auto wasSyncing = bool(d->syncJob);
    if (wasSyncing)
    {
        d->syncJob->abandon();
        d->syncJob = nullptr;
    }

    d->logoutJob = callApi<LogoutJob>();
    emit stateChanged(); // isLoggedIn() == false from now

    connect(d->logoutJob, &LogoutJob::finished, this, [this, wasSyncing] {
        if (d->logoutJob->status().good()
            || d->logoutJob->error() == BaseJob::Unauthorised
            || d->logoutJob->error() == BaseJob::ContentAccessError) {
            if (d->syncLoopConnection)
                disconnect(d->syncLoopConnection);
            d->data->setToken({});
            emit loggedOut();
            deleteLater();
        } else { // logout() somehow didn't proceed - restore the session state
            emit stateChanged();
            if (wasSyncing)
                syncLoopIteration(); // Resume sync loop (or a single sync)
        }
    });
}

void Connection::sync(int timeout)
{
    if (d->syncJob) {
        qCInfo(MAIN) << d->syncJob << "is already running";
        return;
    }
    if (!isLoggedIn()) {
        qCWarning(MAIN) << "Not logged in, not going to sync";
        return;
    }

    d->syncTimeout = timeout;
    Filter filter;
    filter.room.timeline.limit.emplace(100);
    filter.room.state.lazyLoadMembers.emplace(d->lazyLoading);
    auto job = d->syncJob =
        callApi<SyncJob>(BackgroundRequest, d->data->lastEvent(), filter,
                         timeout);
    connect(job, &SyncJob::success, this, [this, job] {
        onSyncSuccess(job->takeData());
        d->syncJob = nullptr;
        emit syncDone();
    });
    connect(job, &SyncJob::retryScheduled, this,
            [this, job](int retriesTaken, int nextInMilliseconds) {
                emit networkError(job->errorString(), job->rawDataSample(),
                                  retriesTaken, nextInMilliseconds);
            });
    connect(job, &SyncJob::failure, this, [this, job] {
        // SyncJob persists with retries on transient errors; if it fails,
        // there's likely something serious enough to stop the loop.
        stopSync();
        if (job->error() == BaseJob::Unauthorised) {
            qCWarning(SYNCJOB)
                << "Sync job failed with Unauthorised - login expired?";
            emit loginError(job->errorString(), job->rawDataSample());
        } else
            emit syncError(job->errorString(), job->rawDataSample());
    });
}

void Connection::syncLoop(int timeout)
{
    if (d->syncLoopConnection && d->syncTimeout == timeout) {
        qCInfo(MAIN) << "Attempt to run sync loop but there's one already "
                        "running; nothing will be done";
        return;
    }
    std::swap(d->syncTimeout, timeout);
    if (d->syncLoopConnection) {
        qCInfo(MAIN) << "Timeout for next syncs changed from"
                        << timeout << "to" << d->syncTimeout;
    } else {
        d->syncLoopConnection = connect(this, &Connection::syncDone,
                                        this, &Connection::syncLoopIteration,
                                        Qt::QueuedConnection);
        syncLoopIteration(); // initial sync to start the loop
    }
}

void Connection::syncLoopIteration()
{
    if (isLoggedIn())
        sync(d->syncTimeout);
    else
        qCInfo(MAIN) << "Logged out, sync loop will stop now";
}

QJsonObject toJson(const DirectChatsMap& directChats)
{
    QJsonObject json;
    for (auto it = directChats.begin(); it != directChats.end();) {
        QJsonArray roomIds;
        const auto* user = it.key();
        for (; it != directChats.end() && it.key() == user; ++it)
            roomIds.append(*it);
        json.insert(user->id(), roomIds);
    }
    return json;
}

void Connection::onSyncSuccess(SyncData&& data, bool fromCache)
{
    d->data->setLastEvent(data.nextBatch());
    d->consumeRoomData(data.takeRoomData(), fromCache);
    d->consumeAccountData(data.takeAccountData());
    d->consumePresenceData(data.takePresenceData());
    d->consumeToDeviceEvents(data.takeToDeviceEvents());
#ifdef Quotient_E2EE_ENABLED
    if(data.deviceOneTimeKeysCount()["signed_curve25519"] < 0.4 * d->olmAccount->maxNumberOfOneTimeKeys() && !d->isUploadingKeys) {
        d->isUploadingKeys = true;
        d->olmAccount->generateOneTimeKeys(d->olmAccount->maxNumberOfOneTimeKeys() - data.deviceOneTimeKeysCount()["signed_curve25519"]);
        auto keys = d->olmAccount->oneTimeKeys();
        auto job = d->olmAccount->createUploadKeyRequest(keys);
        run(job, ForegroundRequest);
        connect(job, &BaseJob::success, this, [=](){
            d->olmAccount->markKeysAsPublished();
        });
        connect(job, &BaseJob::result, this, [=](){
            d->isUploadingKeys = false;
        });
    }
#endif // Quotient_E2EE_ENABLED
}

void Connection::Private::consumeRoomData(SyncDataList&& roomDataList,
                                          bool fromCache)
{
    for (auto&& roomData: roomDataList) {
        const auto forgetIdx = roomIdsToForget.indexOf(roomData.roomId);
        if (forgetIdx != -1) {
            roomIdsToForget.removeAt(forgetIdx);
            if (roomData.joinState == JoinState::Leave) {
                qDebug(MAIN)
                    << "Room" << roomData.roomId
                    << "has been forgotten, ignoring /sync response for it";
                continue;
            }
            qWarning(MAIN) << "Room" << roomData.roomId
                           << "has just been forgotten but /sync returned it in"
                           << toCString(roomData.joinState)
                           << "state - suspiciously fast turnaround";
        }
        if (auto* r = q->provideRoom(roomData.roomId, roomData.joinState)) {
            pendingStateRoomIds.removeOne(roomData.roomId);
            r->updateData(std::move(roomData), fromCache);
            if (firstTimeRooms.removeOne(r)) {
                emit q->loadedRoomState(r);
                if (capabilities.roomVersions)
                    r->checkVersion();
                // Otherwise, the version will be checked in reloadCapabilities()
            }
        }
        // Let UI update itself after updating each room
        QCoreApplication::processEvents();
    }
}

void Connection::Private::consumeAccountData(Events&& accountDataEvents)
{
    // After running this loop, the account data events not saved in
    // accountData (see the end of the loop body) are auto-cleaned away
    for (auto&& eventPtr: accountDataEvents) {
        visit(*eventPtr,
            [this](const DirectChatEvent& dce) {
                // https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
                const auto& usersToDCs = dce.usersToDirectChats();
                DirectChatsMap remoteRemovals =
                    erase_if(directChats, [&usersToDCs, this](auto it) {
                        return !(
                            usersToDCs.contains(it.key()->id(), it.value())
                            || dcLocalAdditions.contains(it.key(), it.value()));
                    });
                erase_if(directChatUsers, [&remoteRemovals](auto it) {
                    return remoteRemovals.contains(it.value(), it.key());
                });
                // Remove from dcLocalRemovals what the server already has.
                erase_if(dcLocalRemovals, [&remoteRemovals](auto it) {
                    return remoteRemovals.contains(it.key(), it.value());
                });
                if (MAIN().isDebugEnabled())
                    for (auto it = remoteRemovals.begin();
                         it != remoteRemovals.end(); ++it) {
                        qCDebug(MAIN)
                            << it.value() << "is no more a direct chat with"
                            << it.key()->id();
                    }

                DirectChatsMap remoteAdditions;
                for (auto it = usersToDCs.begin(); it != usersToDCs.end(); ++it) {
                    if (auto* u = q->user(it.key())) {
                        if (!directChats.contains(u, it.value())
                            && !dcLocalRemovals.contains(u, it.value())) {
                            Q_ASSERT(!directChatUsers.contains(it.value(), u));
                            remoteAdditions.insert(u, it.value());
                            directChats.insert(u, it.value());
                            directChatUsers.insert(it.value(), u);
                            qCDebug(MAIN) << "Marked room" << it.value()
                                          << "as a direct chat with" << u->id();
                        }
                    } else
                        qCWarning(MAIN)
                            << "Couldn't get a user object for" << it.key();
                }
                // Remove from dcLocalAdditions what the server already has.
                erase_if(dcLocalAdditions, [&remoteAdditions](auto it) {
                    return remoteAdditions.contains(it.key(), it.value());
                });
                if (!remoteAdditions.isEmpty() || !remoteRemovals.isEmpty())
                    emit q->directChatsListChanged(remoteAdditions,
                                                   remoteRemovals);
            },
            // catch-all, passing eventPtr for a possible take-over
            [this, &eventPtr](const Event& accountEvent) {
                if (is<IgnoredUsersEvent>(accountEvent))
                    qCDebug(MAIN)
                        << "Users ignored by" << data->userId() << "updated:"
                        << QStringList(q->ignoredUsers().values()).join(',');

                auto& currentData = accountData[accountEvent.matrixType()];
                // A polymorphic event-specific comparison might be a bit
                // more efficient; maaybe do it another day
                if (!currentData
                    || currentData->contentJson() != accountEvent.contentJson()) {
                    currentData = std::move(eventPtr);
                    qCDebug(MAIN) << "Updated account data of type"
                                  << currentData->matrixType();
                    emit q->accountDataChanged(currentData->matrixType());
                }
            });
    }
    if (!dcLocalAdditions.isEmpty() || !dcLocalRemovals.isEmpty()) {
        qDebug(MAIN) << "Sending updated direct chats to the server:"
                     << dcLocalRemovals.size() << "removal(s),"
                     << dcLocalAdditions.size() << "addition(s)";
        q->callApi<SetAccountDataJob>(data->userId(), QStringLiteral("m.direct"),
                                      toJson(directChats));
        dcLocalAdditions.clear();
        dcLocalRemovals.clear();
    }
}

void Connection::Private::consumePresenceData(Events&& presenceData)
{
    // To be implemented
}

void Connection::Private::consumeToDeviceEvents(Events&& toDeviceEvents)
{
#ifdef Quotient_E2EE_ENABLED
    // handling m.room_key to-device encrypted event
    visitEach(toDeviceEvents, [this](const EncryptedEvent& ee) {
        if (ee.algorithm() != OlmV1Curve25519AesSha2AlgoKey) {
            qCDebug(E2EE) << "Encrypted event" << ee.id() << "algorithm"
                          << ee.algorithm() << "is not supported";
            return;
        }

        visit(*sessionDecryptMessage(ee),
            [this, senderKey = ee.senderKey()](const RoomKeyEvent& roomKeyEvent) {
                if (auto* detectedRoom = q->room(roomKeyEvent.roomId())) {
                    detectedRoom->handleRoomKeyEvent(roomKeyEvent, senderKey);
                } else {
                    qCDebug(E2EE)
                        << "Encrypted event room id" << roomKeyEvent.roomId()
                        << "is not found at the connection" << q->objectName();
                }
            },
            [](const Event& evt) {
                qCDebug(E2EE) << "Skipping encrypted to_device event, type"
                              << evt.matrixType();
            });
    });
#endif
}

void Connection::stopSync()
{
    // If there's a sync loop, break it
    disconnect(d->syncLoopConnection);
    if (d->syncJob) // If there's an ongoing sync job, stop it too
    {
        if (d->syncJob->status().code == BaseJob::Pending)
            d->syncJob->abandon();
        d->syncJob = nullptr;
    }
}

QString Connection::nextBatchToken() const { return d->data->lastEvent(); }

PostReceiptJob* Connection::postReceipt(Room* room, RoomEvent* event)
{
    return callApi<PostReceiptJob>(room->id(), "m.read", event->id());
}

JoinRoomJob* Connection::joinRoom(const QString& roomAlias,
                                  const QStringList& serverNames)
{
    auto job = callApi<JoinRoomJob>(roomAlias, serverNames);
    // Upon completion, ensure a room object in Join state is created
    // (or it might already be there due to a sync completing earlier).
    // finished() is used here instead of success() to overtake clients
    // that may add their own slots to finished().
    connect(job, &BaseJob::finished, this, [this, job] {
        if (job->status().good())
            provideRoom(job->roomId());
    });
    return job;
}

LeaveRoomJob* Connection::leaveRoom(Room* room)
{
    const auto& roomId = room->id();
    const auto job = callApi<LeaveRoomJob>(roomId);
    if (room->joinState() == JoinState::Invite) {
        // Workaround matrix-org/synapse#2181 - if the room is in invite state
        // the invite may have been cancelled but Synapse didn't send it in
        // `/sync`. See also #273 for the discussion in the library context.
        d->pendingStateRoomIds.push_back(roomId);
        connect(job, &LeaveRoomJob::success, this, [this, roomId] {
            if (d->pendingStateRoomIds.removeOne(roomId)) {
                qCDebug(MAIN) << "Forcing the room to Leave status";
                provideRoom(roomId, JoinState::Leave);
            }
        });
    }
    return job;
}

inline auto splitMediaId(const QString& mediaId)
{
    auto idParts = mediaId.split('/');
    Q_ASSERT_X(idParts.size() == 2, __FUNCTION__,
               ("'" + mediaId + "' doesn't look like 'serverName/localMediaId'")
                   .toLatin1());
    return idParts;
}

MediaThumbnailJob* Connection::getThumbnail(const QString& mediaId,
                                            QSize requestedSize,
                                            RunningPolicy policy)
{
    auto idParts = splitMediaId(mediaId);
    return callApi<MediaThumbnailJob>(policy, idParts.front(), idParts.back(),
                                      requestedSize);
}

MediaThumbnailJob* Connection::getThumbnail(const QUrl& url, QSize requestedSize,
                                            RunningPolicy policy)
{
    return getThumbnail(url.authority() + url.path(), requestedSize, policy);
}

MediaThumbnailJob* Connection::getThumbnail(const QUrl& url, int requestedWidth,
                                            int requestedHeight,
                                            RunningPolicy policy)
{
    return getThumbnail(url, QSize(requestedWidth, requestedHeight), policy);
}

UploadContentJob*
Connection::uploadContent(QIODevice* contentSource, const QString& filename,
                          const QString& overrideContentType)
{
    Q_ASSERT(contentSource != nullptr);
    auto contentType = overrideContentType;
    if (contentType.isEmpty()) {
        contentType = QMimeDatabase()
                          .mimeTypeForFileNameAndData(filename, contentSource)
                          .name();
        if (!contentSource->open(QIODevice::ReadOnly)) {
            qCWarning(MAIN) << "Couldn't open content source" << filename
                            << "for reading:" << contentSource->errorString();
            return nullptr;
        }
    }
    return callApi<UploadContentJob>(contentSource, filename, contentType);
}

UploadContentJob* Connection::uploadFile(const QString& fileName,
                                         const QString& overrideContentType)
{
    auto sourceFile = new QFile(fileName);
    return uploadContent(sourceFile, QFileInfo(*sourceFile).fileName(),
                         overrideContentType);
}

GetContentJob* Connection::getContent(const QString& mediaId)
{
    auto idParts = splitMediaId(mediaId);
    return callApi<GetContentJob>(idParts.front(), idParts.back());
}

GetContentJob* Connection::getContent(const QUrl& url)
{
    return getContent(url.authority() + url.path());
}

DownloadFileJob* Connection::downloadFile(const QUrl& url,
                                          const QString& localFilename)
{
    auto mediaId = url.authority() + url.path();
    auto idParts = splitMediaId(mediaId);
    auto* job =
        callApi<DownloadFileJob>(idParts.front(), idParts.back(), localFilename);
    return job;
}

CreateRoomJob*
Connection::createRoom(RoomVisibility visibility, const QString& alias,
                       const QString& name, const QString& topic,
                       QStringList invites, const QString& presetName,
                       const QString& roomVersion, bool isDirect,
                       const QVector<CreateRoomJob::StateEvent>& initialState,
                       const QVector<CreateRoomJob::Invite3pid>& invite3pids,
                       const QJsonObject& creationContent)
{
    invites.removeOne(userId()); // The creator is by definition in the room
    auto job = callApi<CreateRoomJob>(visibility == PublishRoom
                                          ? QStringLiteral("public")
                                          : QStringLiteral("private"),
                                      alias, name, topic, invites, invite3pids,
                                      roomVersion, creationContent,
                                      initialState, presetName, isDirect);
    connect(job, &BaseJob::success, this, [this, job, invites, isDirect] {
        auto* room = provideRoom(job->roomId(), JoinState::Join);
        if (!room) {
            Q_ASSERT_X(room, "Connection::createRoom",
                       "Failed to create a room");
            return;
        }
        emit createdRoom(room);
        if (isDirect)
            for (const auto& i : invites)
                addToDirectChats(room, user(i));
    });
    return job;
}

void Connection::requestDirectChat(const QString& userId)
{
    doInDirectChat(userId, [this](Room* r) { emit directChatAvailable(r); });
}

void Connection::requestDirectChat(User* u)
{
    doInDirectChat(u, [this](Room* r) { emit directChatAvailable(r); });
}

void Connection::doInDirectChat(const QString& userId,
                                const std::function<void(Room*)>& operation)
{
    if (auto* u = user(userId))
        doInDirectChat(u, operation);
    else
        qCCritical(MAIN)
            << "Connection::doInDirectChat: Couldn't get a user object for"
            << userId;
}

void Connection::doInDirectChat(User* u,
                                const std::function<void(Room*)>& operation)
{
    Q_ASSERT(u);
    const auto& otherUserId = u->id();
    // There can be more than one DC; find the first valid (existing and
    // not left), and delete inexistent (forgotten?) ones along the way.
    DirectChatsMap removals;
    for (auto it = d->directChats.constFind(u);
         it != d->directChats.cend() && it.key() == u; ++it) {
        const auto& roomId = *it;
        if (auto r = room(roomId, JoinState::Join)) {
            Q_ASSERT(r->id() == roomId);
            // A direct chat with yourself should only involve yourself :)
            if (otherUserId == userId() && r->totalMemberCount() > 1)
                continue;
            qCDebug(MAIN) << "Requested direct chat with" << otherUserId
                          << "is already available as" << r->id();
            operation(r);
            return;
        }
        if (auto ir = invitation(roomId)) {
            Q_ASSERT(ir->id() == roomId);
            auto j = joinRoom(ir->id());
            connect(j, &BaseJob::success, this,
                    [this, roomId, otherUserId, operation] {
                        qCDebug(MAIN)
                            << "Joined the already invited direct chat with"
                            << otherUserId << "as" << roomId;
                        operation(room(roomId, JoinState::Join));
                    });
            return;
        }
        // Avoid reusing previously left chats but don't remove them
        // from direct chat maps, either.
        if (room(roomId, JoinState::Leave))
            continue;

        qCWarning(MAIN) << "Direct chat with" << otherUserId << "known as room"
                        << roomId << "is not valid and will be discarded";
        // Postpone actual deletion until we finish iterating d->directChats.
        removals.insert(it.key(), it.value());
        // Add to the list of updates to send to the server upon the next sync.
        d->dcLocalRemovals.insert(it.key(), it.value());
    }
    if (!removals.isEmpty()) {
        for (auto it = removals.cbegin(); it != removals.cend(); ++it) {
            d->directChats.remove(it.key(), it.value());
            d->directChatUsers.remove(it.value(),
                                      const_cast<User*>(it.key())); // FIXME
        }
        emit directChatsListChanged({}, removals);
    }

    auto j = createDirectChat(otherUserId);
    connect(j, &BaseJob::success, this, [this, j, otherUserId, operation] {
        qCDebug(MAIN) << "Direct chat with" << otherUserId << "has been created as"
                      << j->roomId();
        operation(room(j->roomId(), JoinState::Join));
    });
}

CreateRoomJob* Connection::createDirectChat(const QString& userId,
                                            const QString& topic,
                                            const QString& name)
{
    return createRoom(UnpublishRoom, {}, name, topic, { userId },
                      QStringLiteral("trusted_private_chat"), {}, true);
}

ForgetRoomJob* Connection::forgetRoom(const QString& id)
{
    // To forget is hard :) First we should ensure the local user is not
    // in the room (by leaving it, if necessary); once it's done, the /forget
    // endpoint can be called; and once this is through, the local Room object
    // (if any existed) is deleted. At the same time, we still have to
    // (basically immediately) return a pointer to ForgetRoomJob. Therefore
    // a ForgetRoomJob is created in advance and can be returned in a probably
    // not-yet-started state (it will start once /leave completes).
    auto forgetJob = new ForgetRoomJob(id);
    auto room = d->roomMap.value({ id, false });
    if (!room)
        room = d->roomMap.value({ id, true });
    if (room && room->joinState() != JoinState::Leave) {
        auto leaveJob = leaveRoom(room);
        connect(leaveJob, &BaseJob::result, this,
                [this, leaveJob, forgetJob, room] {
                    if (leaveJob->error() == BaseJob::Success
                        || leaveJob->error() == BaseJob::NotFoundError) {
                        run(forgetJob);
                        // If the matching /sync response hasn't arrived yet,
                        // mark the room for explicit deletion
                        if (room->joinState() != JoinState::Leave)
                            d->roomIdsToForget.push_back(room->id());
                    } else {
                        qCWarning(MAIN).nospace()
                            << "Error leaving room " << room->objectName()
                            << ": " << leaveJob->errorString();
                        forgetJob->abandon();
                    }
                });
        connect(leaveJob, &BaseJob::failure, forgetJob, &BaseJob::abandon);
    } else
        run(forgetJob);
    connect(forgetJob, &BaseJob::result, this, [this, id, forgetJob] {
        // Leave room in case of success, or room not known by server
        if (forgetJob->error() == BaseJob::Success
            || forgetJob->error() == BaseJob::NotFoundError)
            d->removeRoom(id); // Delete the room from roomMap
        else
            qCWarning(MAIN).nospace() << "Error forgetting room " << id << ": "
                                      << forgetJob->errorString();
    });
    return forgetJob;
}

SendToDeviceJob*
Connection::sendToDevices(const QString& eventType,
                          const UsersToDevicesToEvents& eventsMap)
{
    QHash<QString, QHash<QString, QJsonObject>> json;
    json.reserve(int(eventsMap.size()));
    std::for_each(eventsMap.begin(), eventsMap.end(),
                  [&json](const auto& userTodevicesToEvents) {
                      auto& jsonUser = json[userTodevicesToEvents.first];
                      const auto& devicesToEvents = userTodevicesToEvents.second;
                      std::for_each(devicesToEvents.begin(),
                                    devicesToEvents.end(),
                                    [&jsonUser](const auto& deviceToEvents) {
                                        jsonUser.insert(
                                            deviceToEvents.first,
                                            deviceToEvents.second.contentJson());
                                    });
                  });
    return callApi<SendToDeviceJob>(BackgroundRequest, eventType,
                                    generateTxnId(), json);
}

SendMessageJob* Connection::sendMessage(const QString& roomId,
                                        const RoomEvent& event)
{
    const auto txnId = event.transactionId().isEmpty() ? generateTxnId()
                                                       : event.transactionId();
    return callApi<SendMessageJob>(roomId, event.matrixType(), txnId,
                                   event.contentJson());
}

QUrl Connection::homeserver() const { return d->data->baseUrl(); }

QString Connection::domain() const { return userId().section(':', 1); }

bool Connection::isUsable() const { return !loginFlows().isEmpty(); }

QVector<GetLoginFlowsJob::LoginFlow> Connection::loginFlows() const
{
    return d->loginFlows;
}

bool Connection::supportsPasswordAuth() const
{
    return d->loginFlows.contains(LoginFlows::Password);
}

bool Connection::supportsSso() const
{
    return d->loginFlows.contains(LoginFlows::SSO);
}

Room* Connection::room(const QString& roomId, JoinStates states) const
{
    Room* room = d->roomMap.value({ roomId, false }, nullptr);
    if (states.testFlag(JoinState::Join) && room
        && room->joinState() == JoinState::Join)
        return room;

    if (states.testFlag(JoinState::Invite))
        if (Room* invRoom = invitation(roomId))
            return invRoom;

    if (states.testFlag(JoinState::Leave) && room
        && room->joinState() == JoinState::Leave)
        return room;

    return nullptr;
}

Room* Connection::roomByAlias(const QString& roomAlias, JoinStates states) const
{
    const auto id = d->roomAliasMap.value(roomAlias);
    if (!id.isEmpty())
        return room(id, states);

    qCWarning(MAIN) << "Room for alias" << roomAlias
                    << "is not found under account" << userId();
    return nullptr;
}

void Connection::updateRoomAliases(const QString& roomId,
                                   const QStringList& previousRoomAliases,
                                   const QStringList& roomAliases)
{
    for (const auto& a : previousRoomAliases)
        if (d->roomAliasMap.remove(a) == 0)
            qCWarning(MAIN) << "Alias" << a << "is not found (already deleted?)";

    for (const auto& a : roomAliases) {
        auto& mappedId = d->roomAliasMap[a];
        if (!mappedId.isEmpty()) {
            if (mappedId == roomId)
                qCDebug(MAIN)
                    << "Alias" << a << "is already mapped to" << roomId;
            else
                qCWarning(MAIN) << "Alias" << a << "will be force-remapped from"
                                << mappedId << "to" << roomId;
        }
        mappedId = roomId;
    }
}

Room* Connection::invitation(const QString& roomId) const
{
    return d->roomMap.value({ roomId, true }, nullptr);
}

User* Connection::user(const QString& uId)
{
    if (uId.isEmpty())
        return nullptr;
    if (!uId.startsWith('@') || serverPart(uId).isEmpty()) {
        qCCritical(MAIN) << "Malformed userId:" << uId;
        return nullptr;
    }
    if (d->userMap.contains(uId))
        return d->userMap.value(uId);
    auto* user = userFactory()(this, uId);
    d->userMap.insert(uId, user);
    emit newUser(user);
    return user;
}

const User* Connection::user() const
{
    return d->userMap.value(userId(), nullptr);
}

User* Connection::user() { return user(userId()); }

QString Connection::userId() const { return d->data->userId(); }

QString Connection::deviceId() const { return d->data->deviceId(); }

QByteArray Connection::accessToken() const
{
    // The logout job needs access token to do its job; so the token is
    // kept inside d->data but no more exposed to the outside world.
    return isJobPending(d->logoutJob) ? QByteArray() : d->data->accessToken();
}

bool Connection::isLoggedIn() const { return !accessToken().isEmpty(); }

#ifdef Quotient_E2EE_ENABLED
QOlmAccount *Connection::olmAccount() const
{
    return d->olmAccount.get(); //d->encryptionManager->account();
}
#endif // Quotient_E2EE_ENABLED

SyncJob* Connection::syncJob() const { return d->syncJob; }

int Connection::millisToReconnect() const
{
    return d->syncJob ? d->syncJob->millisToRetry() : 0;
}

QHash<QPair<QString, bool>, Room*> Connection::roomMap() const
{
    // Copy-on-write-and-remove-elements is faster than copying elements one by
    // one.
    QHash<QPair<QString, bool>, Room*> roomMap = d->roomMap;
    for (auto it = roomMap.begin(); it != roomMap.end();) {
        if (it.value()->joinState() == JoinState::Leave)
            it = roomMap.erase(it);
        else
            ++it;
    }
    return roomMap;
}

QVector<Room*> Connection::allRooms() const
{
    QVector<Room*> result;
    result.resize(d->roomMap.size());
    std::copy(d->roomMap.cbegin(), d->roomMap.cend(), result.begin());
    return result;
}

QVector<Room*> Connection::rooms(JoinStates joinStates) const
{
    QVector<Room*> result;
    for (auto* r: qAsConst(d->roomMap))
        if (joinStates.testFlag(r->joinState()))
            result.push_back(r);
    return result;
}

int Connection::roomsCount(JoinStates joinStates) const
{
    // Using int to maintain compatibility with QML
    // (consider also that QHash<>::size() returns int anyway).
    return int(std::count_if(d->roomMap.cbegin(), d->roomMap.cend(),
                             [joinStates](Room* r) {
                                 return joinStates.testFlag(r->joinState());
                             }));
}

bool Connection::hasAccountData(const QString& type) const
{
    return d->accountData.find(type) != d->accountData.cend();
}

const EventPtr& Connection::accountData(const QString& type) const
{
    static EventPtr NoEventPtr {};
    auto it = d->accountData.find(type);
    return it == d->accountData.end() ? NoEventPtr : it->second;
}

QJsonObject Connection::accountDataJson(const QString& type) const
{
    const auto& eventPtr = accountData(type);
    return eventPtr ? eventPtr->contentJson() : QJsonObject();
}

void Connection::setAccountData(EventPtr&& event)
{
    d->packAndSendAccountData(std::move(event));
}

void Connection::setAccountData(const QString& type, const QJsonObject& content)
{
    d->packAndSendAccountData(loadEvent<Event>(type, content));
}

QHash<QString, QVector<Room*>> Connection::tagsToRooms() const
{
    QHash<QString, QVector<Room*>> result;
    for (auto* r : qAsConst(d->roomMap)) {
        const auto& tagNames = r->tagNames();
        for (const auto& tagName : tagNames)
            result[tagName].push_back(r);
    }
    for (auto it = result.begin(); it != result.end(); ++it)
        std::sort(it->begin(), it->end(), [t = it.key()](Room* r1, Room* r2) {
            return r1->tags().value(t) < r2->tags().value(t);
        });
    return result;
}

QStringList Connection::tagNames() const
{
    QStringList tags({ FavouriteTag });
    for (auto* r : qAsConst(d->roomMap)) {
        const auto& tagNames = r->tagNames();
        for (const auto& tag : tagNames)
            if (tag != LowPriorityTag && !tags.contains(tag))
                tags.push_back(tag);
    }
    tags.push_back(LowPriorityTag);
    return tags;
}

QVector<Room*> Connection::roomsWithTag(const QString& tagName) const
{
    QVector<Room*> rooms;
    std::copy_if(d->roomMap.cbegin(), d->roomMap.cend(),
                 std::back_inserter(rooms),
                 [&tagName](Room* r) { return r->tags().contains(tagName); });
    return rooms;
}

DirectChatsMap Connection::directChats() const
{
    return d->directChats;
}

// Removes room with given id from roomMap
void Connection::Private::removeRoom(const QString& roomId)
{
    for (auto f : { false, true })
        if (auto r = roomMap.take({ roomId, f })) {
            qCDebug(MAIN) << "Room" << r->objectName() << "in state"
                          << toCString(r->joinState()) << "will be deleted";
            emit r->beforeDestruction(r);
            r->deleteLater();
        }
}

void Connection::addToDirectChats(const Room* room, User* user)
{
    Q_ASSERT(room != nullptr && user != nullptr);
    if (d->directChats.contains(user, room->id()))
        return;
    Q_ASSERT(!d->directChatUsers.contains(room->id(), user));
    d->directChats.insert(user, room->id());
    d->directChatUsers.insert(room->id(), user);
    d->dcLocalAdditions.insert(user, room->id());
    emit directChatsListChanged({ { user, room->id() } }, {});
}

void Connection::removeFromDirectChats(const QString& roomId, User* user)
{
    Q_ASSERT(!roomId.isEmpty());
    if ((user != nullptr && !d->directChats.contains(user, roomId))
        || d->directChats.key(roomId) == nullptr)
        return;

    DirectChatsMap removals;
    if (user != nullptr) {
        d->directChats.remove(user, roomId);
        d->directChatUsers.remove(roomId, user);
        removals.insert(user, roomId);
        d->dcLocalRemovals.insert(user, roomId);
    } else {
        removals = erase_if(d->directChats,
                            [&roomId](auto it) { return it.value() == roomId; });
        d->directChatUsers.remove(roomId);
        d->dcLocalRemovals += removals;
    }
    emit directChatsListChanged({}, removals);
}

bool Connection::isDirectChat(const QString& roomId) const
{
    return d->directChatUsers.contains(roomId);
}

QList<User*> Connection::directChatUsers(const Room* room) const
{
    Q_ASSERT(room != nullptr);
    return d->directChatUsers.values(room->id());
}

bool Connection::isIgnored(const User* user) const
{
    return ignoredUsers().contains(user->id());
}

IgnoredUsersList Connection::ignoredUsers() const
{
    const auto* event = d->unpackAccountData<IgnoredUsersEvent>();
    return event ? event->ignored_users() : IgnoredUsersList();
}

void Connection::addToIgnoredUsers(const User* user)
{
    Q_ASSERT(user != nullptr);

    auto ignoreList = ignoredUsers();
    if (!ignoreList.contains(user->id())) {
        ignoreList.insert(user->id());
        d->packAndSendAccountData<IgnoredUsersEvent>(ignoreList);
        emit ignoredUsersListChanged({ { user->id() } }, {});
    }
}

void Connection::removeFromIgnoredUsers(const User* user)
{
    Q_ASSERT(user != nullptr);

    auto ignoreList = ignoredUsers();
    if (ignoreList.remove(user->id()) != 0) {
        d->packAndSendAccountData<IgnoredUsersEvent>(ignoreList);
        emit ignoredUsersListChanged({}, { { user->id() } });
    }
}

QMap<QString, User*> Connection::users() const { return d->userMap; }

const ConnectionData* Connection::connectionData() const
{
    return d->data.get();
}

Room* Connection::provideRoom(const QString& id, Omittable<JoinState> joinState)
{
    // TODO: This whole function is a strong case for a RoomManager class.
    Q_ASSERT_X(!id.isEmpty(), __FUNCTION__, "Empty room id");

    // If joinState is empty, all joinState == comparisons below are false.
    const auto roomKey = qMakePair(id, joinState == JoinState::Invite);
    auto* room = d->roomMap.value(roomKey, nullptr);
    if (room) {
        // Leave is a special case because in transition (5a) (see the .h file)
        // joinState == room->joinState but we still have to preempt the Invite
        // and emit a signal. For Invite and Join, there's no such problem.
        if (room->joinState() == joinState && joinState != JoinState::Leave)
            return room;
    } else if (!joinState) {
        // No Join and Leave, maybe Invite?
        room = d->roomMap.value({ id, true }, nullptr);
        if (room)
            return room;
        // No Invite either, setup a new room object below
    }

    if (!room) {
        room = roomFactory()(this, id, joinState.value_or(JoinState::Join));
        if (!room) {
            qCCritical(MAIN) << "Failed to create a room" << id;
            return nullptr;
        }
        d->roomMap.insert(roomKey, room);
        d->firstTimeRooms.push_back(room);
        connect(room, &Room::beforeDestruction, this,
                &Connection::aboutToDeleteRoom);
        emit newRoom(room);
    }
    if (!joinState)
        return room;

    if (*joinState == JoinState::Invite) {
        // prev is either Leave or nullptr
        auto* prev = d->roomMap.value({ id, false }, nullptr);
        emit invitedRoom(room, prev);
    } else {
        room->setJoinState(*joinState);
        // Preempt the Invite room (if any) with a room in Join/Leave state.
        auto* prevInvite = d->roomMap.take({ id, true });
        if (*joinState == JoinState::Join)
            emit joinedRoom(room, prevInvite);
        else if (*joinState == JoinState::Leave)
            emit leftRoom(room, prevInvite);
        if (prevInvite) {
            const auto dcUsers = prevInvite->directChatUsers();
            for (auto* u : dcUsers)
                addToDirectChats(room, u);
            qCDebug(MAIN) << "Deleting Invite state for room"
                          << prevInvite->id();
            emit prevInvite->beforeDestruction(prevInvite);
            prevInvite->deleteLater();
        }
    }

    return room;
}

void Connection::setRoomFactory(room_factory_t f)
{
    _roomFactory = std::move(f);
}

void Connection::setUserFactory(user_factory_t f)
{
    _userFactory = std::move(f);
}

room_factory_t Connection::roomFactory() { return _roomFactory; }

user_factory_t Connection::userFactory() { return _userFactory; }

room_factory_t Connection::_roomFactory = defaultRoomFactory<>();
user_factory_t Connection::_userFactory = defaultUserFactory<>();

QByteArray Connection::generateTxnId() const
{
    return d->data->generateTxnId();
}

void Connection::setHomeserver(const QUrl& url)
{
    if (isJobPending(d->resolverJob))
        d->resolverJob->abandon();
    if (isJobPending(d->loginFlowsJob))
        d->loginFlowsJob->abandon();
    d->loginFlows.clear();

    if (homeserver() != url) {
        d->data->setBaseUrl(url);
        emit homeserverChanged(homeserver());
    }

    // Whenever a homeserver is updated, retrieve available login flows from it
    d->loginFlowsJob = callApi<GetLoginFlowsJob>(BackgroundRequest);
    connect(d->loginFlowsJob, &BaseJob::result, this, [this] {
        if (d->loginFlowsJob->status().good())
            d->loginFlows = d->loginFlowsJob->flows();
        else
            d->loginFlows.clear();
        emit loginFlowsChanged();
    });
}

void Connection::saveRoomState(Room* r) const
{
    Q_ASSERT(r);
    if (!d->cacheState)
        return;

    QFile outRoomFile { stateCacheDir().filePath(
        SyncData::fileNameForRoom(r->id())) };
    if (outRoomFile.open(QFile::WriteOnly)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        const auto data =
            d->cacheToBinary
                ? QCborValue::fromJsonValue(r->toJson()).toCbor()
                : QJsonDocument(r->toJson()).toJson(QJsonDocument::Compact);
#else
        QJsonDocument json { r->toJson() };
        const auto data = d->cacheToBinary ? json.toBinaryData()
                                           : json.toJson(QJsonDocument::Compact);
#endif
        outRoomFile.write(data.data(), data.size());
        qCDebug(MAIN) << "Room state cache saved to" << outRoomFile.fileName();
    } else {
        qCWarning(MAIN) << "Error opening" << outRoomFile.fileName() << ":"
                        << outRoomFile.errorString();
    }
}

void Connection::saveState() const
{
    if (!d->cacheState)
        return;

    QElapsedTimer et;
    et.start();

    QFile outFile { d->topLevelStatePath() };
    if (!outFile.open(QFile::WriteOnly)) {
        qCWarning(MAIN) << "Error opening" << outFile.fileName() << ":"
                        << outFile.errorString();
        qCWarning(MAIN) << "Caching the rooms state disabled";
        d->cacheState = false;
        return;
    }

    QJsonObject rootObj {
        { QStringLiteral("cache_version"),
          QJsonObject {
              { QStringLiteral("major"), SyncData::cacheVersion().first },
              { QStringLiteral("minor"), SyncData::cacheVersion().second } } }
    };
    {
        QJsonObject roomsJson;
        QJsonObject inviteRoomsJson;
        for (const auto* r: qAsConst(d->roomMap)) {
            if (r->joinState() == JoinState::Leave)
                continue;
            (r->joinState() == JoinState::Invite ? inviteRoomsJson : roomsJson)
                .insert(r->id(), QJsonValue::Null);
        }

        QJsonObject roomObj;
        if (!roomsJson.isEmpty())
            roomObj.insert(QStringLiteral("join"), roomsJson);
        if (!inviteRoomsJson.isEmpty())
            roomObj.insert(QStringLiteral("invite"), inviteRoomsJson);

        rootObj.insert(QStringLiteral("next_batch"), d->data->lastEvent());
        rootObj.insert(QStringLiteral("rooms"), roomObj);
    }
    {
        QJsonArray accountDataEvents {
            basicEventJson(QStringLiteral("m.direct"), toJson(d->directChats))
        };
        for (const auto& e : d->accountData)
            accountDataEvents.append(
                basicEventJson(e.first, e.second->contentJson()));

        rootObj.insert(QStringLiteral("account_data"),
                       QJsonObject {
                           { QStringLiteral("events"), accountDataEvents } });
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    const auto data =
        d->cacheToBinary ? QCborValue::fromJsonValue(rootObj).toCbor()
                         : QJsonDocument(rootObj).toJson(QJsonDocument::Compact);
#else
    QJsonDocument json { rootObj };
    const auto data = d->cacheToBinary ? json.toBinaryData()
                                       : json.toJson(QJsonDocument::Compact);
#endif
    qCDebug(PROFILER) << "Cache for" << userId() << "generated in" << et;

    outFile.write(data.data(), data.size());
    qCDebug(MAIN) << "State cache saved to" << outFile.fileName();
}

void Connection::loadState()
{
    if (!d->cacheState)
        return;

    QElapsedTimer et;
    et.start();

    SyncData sync { d->topLevelStatePath() };
    if (sync.nextBatch().isEmpty()) // No token means no cache by definition
        return;

    if (!sync.unresolvedRooms().isEmpty()) {
        qCWarning(MAIN) << "State cache incomplete, discarding";
        return;
    }
    // TODO: to handle load failures, instead of the above block:
    // 1. Do initial sync on failed rooms without saving the nextBatch token
    // 2. Do the sync across all rooms as normal
    onSyncSuccess(std::move(sync), true);
    qCDebug(PROFILER) << "*** Cached state for" << userId() << "loaded in" << et;
}

QString Connection::stateCachePath() const
{
    return stateCacheDir().path() % '/';
}

QDir Connection::stateCacheDir() const
{
    auto safeUserId = userId();
    safeUserId.replace(':', '_');
    return cacheLocation(safeUserId);
}

bool Connection::cacheState() const { return d->cacheState; }

void Connection::setCacheState(bool newValue)
{
    if (d->cacheState != newValue) {
        d->cacheState = newValue;
        emit cacheStateChanged();
    }
}

bool Connection::lazyLoading() const { return d->lazyLoading; }

void Connection::setLazyLoading(bool newValue)
{
    if (d->lazyLoading != newValue) {
        d->lazyLoading = newValue;
        emit lazyLoadingChanged();
    }
}

BaseJob* Connection::run(BaseJob* job, RunningPolicy runningPolicy)
{
    // Reparent to protect from #397, #398 and to prevent BaseJob* from being
    // garbage-collected if made by or returned to QML/JavaScript.
    job->setParent(this);
    connect(job, &BaseJob::failure, this, &Connection::requestFailed);
    job->initiate(d->data.get(), runningPolicy & BackgroundRequest);
    return job;
}

void Connection::getTurnServers()
{
    auto job = callApi<GetTurnServerJob>();
    connect(job, &GetTurnServerJob::success, this,
            [=] { emit turnServersChanged(job->data()); });
}

const QString Connection::SupportedRoomVersion::StableTag =
    QStringLiteral("stable");

QString Connection::defaultRoomVersion() const
{
    return d->capabilities.roomVersions
               ? d->capabilities.roomVersions->defaultVersion
               : QString();
}

QStringList Connection::stableRoomVersions() const
{
    QStringList l;
    if (d->capabilities.roomVersions) {
        const auto& allVersions = d->capabilities.roomVersions->available;
        for (auto it = allVersions.begin(); it != allVersions.end(); ++it)
            if (it.value() == SupportedRoomVersion::StableTag)
                l.push_back(it.key());
    }
    return l;
}

bool Connection::canChangePassword() const
{
    // By default assume we can
    return d->capabilities.changePassword
           ? d->capabilities.changePassword->enabled
           : true;
}

inline bool roomVersionLess(const Connection::SupportedRoomVersion& v1,
                            const Connection::SupportedRoomVersion& v2)
{
    bool ok1 = false, ok2 = false;
    const auto vNum1 = v1.id.toFloat(&ok1);
    const auto vNum2 = v2.id.toFloat(&ok2);
    return ok1 && ok2 ? vNum1 < vNum2 : v1.id < v2.id;
}

QVector<Connection::SupportedRoomVersion> Connection::availableRoomVersions() const
{
    QVector<SupportedRoomVersion> result;
    if (d->capabilities.roomVersions) {
        const auto& allVersions = d->capabilities.roomVersions->available;
        result.reserve(allVersions.size());
        for (auto it = allVersions.begin(); it != allVersions.end(); ++it)
            result.push_back({ it.key(), it.value() });
        // Put stable versions over unstable; within each group,
        // sort numeric versions as numbers, the rest as strings.
        const auto mid =
            std::partition(result.begin(), result.end(),
                           std::mem_fn(&SupportedRoomVersion::isStable));
        std::sort(result.begin(), mid, roomVersionLess);
        std::sort(mid, result.end(), roomVersionLess);
    }
    return result;
}

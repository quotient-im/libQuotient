// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2019 Ville Ranki <ville.ranki@iki.fi>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "connection.h"

#include "accountregistry.h"
#include "connectiondata.h"
#include "qt_connection_util.h"
#include "room.h"
#include "settings.h"
#include "user.h"
#include "connection_p.h"

// NB: since Qt 6, moc_connection.cpp needs Room and User fully defined
#include "moc_connection.cpp"

#include "csapi/account-data.h"
#include "csapi/capabilities.h"
#include "csapi/joining.h"
#include "csapi/leaving.h"
#include "csapi/logout.h"
#include "csapi/room_send.h"
#include "csapi/to_device.h"
#include "csapi/voip.h"
#include "csapi/wellknown.h"
#include "csapi/whoami.h"

#include "events/directchatevent.h"
#include "jobs/downloadfilejob.h"
#include "jobs/mediathumbnailjob.h"
#include "jobs/syncjob.h"
#include <variant>

#ifdef Quotient_E2EE_ENABLED
#    include "database.h"
#    include "keyverificationsession.h"

#    include "e2ee/qolmaccount.h"
#    include "e2ee/qolminboundsession.h"
#    include "e2ee/qolmsession.h"
#    include "e2ee/qolmutility.h"

#    include "events/keyverificationevent.h"
#endif // Quotient_E2EE_ENABLED

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
HashT remove_if(HashT& hashMap, Pred pred)
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

Connection::Connection(const QUrl& server, QObject* parent)
    : QObject(parent)
    , d(makeImpl<Private>(std::make_unique<ConnectionData>(server)))
{
#ifdef Quotient_E2EE_ENABLED
    //connect(qApp, &QCoreApplication::aboutToQuit, this, &Connection::saveOlmAccount);
#endif
    d->q = this; // All d initialization should occur before this line
    setObjectName(server.toString());
}

Connection::Connection(QObject* parent) : Connection({}, parent) {}

Connection::~Connection()
{
    qCDebug(MAIN) << "deconstructing connection object for" << userId();
    stopSync();
    Accounts.drop(this);
}

void Connection::resolveServer(const QString& mxid)
{
    if (isJobPending(d->resolverJob))
        d->resolverJob->abandon();

    auto maybeBaseUrl = QUrl::fromUserInput(serverPart(mxid));
    maybeBaseUrl.setScheme("https"_ls); // Instead of the Qt-default "http"
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

        if (d->resolverJob->error() != BaseJob::NotFound) {
            if (!d->resolverJob->status().good()) {
                qCWarning(MAIN)
                    << "Fetching .well-known file failed, FAIL_PROMPT";
                emit resolveError(tr("Failed resolving the homeserver"));
                return;
            }
            const QUrl baseUrl { d->resolverJob->data().homeserver.baseUrl };
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
    d->checkAndConnect(userId, [=,this] {
        d->loginToServer(LoginFlows::Password.type, makeUserIdentifier(userId),
                         password, /*token*/ QString(), deviceId, initialDeviceName);
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
                     none /*user is encoded in loginToken*/, QString() /*password*/,
                     QString::fromUtf8(loginToken), deviceId, initialDeviceName);
}

void Connection::assumeIdentity(const QString& mxId, const QString& accessToken,
                                [[maybe_unused]] const QString& deviceId)
{
    assumeIdentity(mxId, accessToken);
}

void Connection::assumeIdentity(const QString& mxId, const QString& accessToken)
{
    d->checkAndConnect(mxId, [this, mxId, accessToken] {
        d->data->setToken(accessToken.toLatin1());
        auto* job = callApi<GetTokenOwnerJob>();
        connect(job, &BaseJob::success, this, [this, job, mxId] {
            if (mxId != job->userId())
                qCWarning(MAIN).nospace()
                    << "The access_token owner (" << job->userId()
                    << ") is different from passed MXID (" << mxId << ")!";
            d->data->setDeviceId(job->deviceId());
            d->completeSetup(job->userId());
        });
        connect(job, &BaseJob::failure, this, [this, job] {
            if (job->error() == BaseJob::StatusCode::NetworkError)
                emit networkError(job->errorString(), job->rawDataSample(),
                                  job->maxRetries(), -1);
            else
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
        if (d->capabilitiesJob->error() == BaseJob::IncorrectRequest)
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

void Connection::Private::saveAccessTokenToKeychain() const
{
    qCDebug(MAIN) << "Saving access token to keychain for" << q->userId();
    auto job = new QKeychain::WritePasswordJob(qAppName());
    job->setKey(q->userId());
    job->setBinaryData(data->accessToken());
    job->start();
    QObject::connect(job, &QKeychain::Job::finished, q, [job] {
        if (job->error() == QKeychain::Error::NoError)
            return;
        qWarning().noquote()
            << "Could not save access token to the keychain:"
            << qUtf8Printable(job->errorString());
        // TODO: emit a signal
    });

}

void Connection::Private::dropAccessToken()
{
    // TODO: emit a signal on important (i.e. access denied) keychain errors
    using namespace QKeychain;
    qCDebug(MAIN) << "Removing access token from keychain for" << q->userId();
    auto job = new DeletePasswordJob(qAppName());
    job->setKey(q->userId());
    job->start();
    QObject::connect(job, &Job::finished, q, [job] {
        if (job->error() == Error::NoError
            || job->error() == Error::EntryNotFound)
            return;
        qWarning().noquote()
            << "Could not delete access token from the keychain:"
            << qUtf8Printable(job->errorString());
    });

    auto pickleJob = new DeletePasswordJob(qAppName());
    pickleJob->setKey(q->userId() + "-Pickle"_ls);
    pickleJob->start();
    QObject::connect(job, &Job::finished, q, [job] {
        if (job->error() == Error::NoError
            || job->error() == Error::EntryNotFound)
            return;
        qWarning().noquote()
            << "Could not delete account pickle from the keychain:"
            << qUtf8Printable(job->errorString());
    });

    data->setToken({});
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
        saveAccessTokenToKeychain();
#ifdef Quotient_E2EE_ENABLED
//        database->clear();
#endif
    });
    connect(loginJob, &BaseJob::failure, q, [this, loginJob] {
        emit q->loginError(loginJob->errorString(), loginJob->rawDataSample());
    });
}

#ifdef Quotient_E2EE_ENABLED
Expected<PicklingKey, QKeychain::Error> setupPicklingKey(const QString& userId)
{
    // TODO: Rewrite the whole thing in an async way to get rid of nested event
    // loops; maybe move the function to Connection::Private to emit a signal
    // when the work completes (successfully or unsuccessfully).
    using namespace QKeychain;
    AccountSettings accountSettings(userId);
    ReadPasswordJob readJob(qAppName());
    readJob.setAutoDelete(false);
    readJob.setKey(accountSettings.userId() + QStringLiteral("-Pickle"));
    QEventLoop readLoop;
    QObject::connect(&readJob, &Job::finished, &readLoop, &QEventLoop::quit);
    readJob.start();
    readLoop.exec();

    if (readJob.error() == Error::NoError) {
        qDebug(E2EE) << "Successfully loaded pickling key from keychain";
        return PicklingKey::fromByteArray(readJob.binaryData());
    }
    if (readJob.error() == Error::EntryNotFound) {
        auto&& picklingKey = PicklingKey::generate();
        WritePasswordJob writeJob(qAppName());
        writeJob.setAutoDelete(false);
        writeJob.setKey(accountSettings.userId() + QStringLiteral("-Pickle"));
        writeJob.setBinaryData(picklingKey.viewAsByteArray());
        QEventLoop writeLoop;
        QObject::connect(&writeJob, &Job::finished, &writeLoop,
                         &QEventLoop::quit);
        writeJob.start();
        writeLoop.exec();

        if (writeJob.error() == Error::NoError)
            return std::move(picklingKey);

        qCritical(E2EE) << "Could not save pickling key to keychain: "
                        << writeJob.errorString();
        qCritical(E2EE) << "Pickles in the database will be unencrypted";
        return writeJob.error();
    }
    qWarning(E2EE) << "Error loading pickling key - please fix your keychain:"
                    << readJob.errorString();
    return readJob.error();
}
#endif

void Connection::Private::completeSetup(const QString& mxId)
{
    data->setUserId(mxId);
    q->user(); // Creates a User object for the local user
    q->setObjectName(data->userId() % u'/' % data->deviceId());
    qCDebug(MAIN) << "Using server" << data->baseUrl().toDisplayString()
                  << "by user" << data->userId()
                  << "from device" << data->deviceId();
    Accounts.add(q);
    connect(qApp, &QCoreApplication::aboutToQuit, q, &Connection::saveState);
#ifndef Quotient_E2EE_ENABLED
    qCWarning(E2EE) << "End-to-end encryption (E2EE) support is turned off.";
#else // Quotient_E2EE_ENABLED
    auto&& maybePicklingKey = setupPicklingKey(data->userId());
    if (!maybePicklingKey) {
        Q_ASSERT(maybePicklingKey.has_value());
        qCritical(E2EE) << "Could not load or initialise a pickling key, will "
                           "use a mock key for pickling";
    }
    database =
        new Database(data->userId(), data->deviceId(),
                     maybePicklingKey.move_value_or(PicklingKey::mock()),
                     q);

    olmAccount = std::make_unique<QOlmAccount>(data->userId(), data->deviceId(), q);
    connect(olmAccount.get(), &QOlmAccount::needsSave, q,
            [this] { saveOlmAccount(); });

    loadSessions();

    if (const auto outcome = database->setupOlmAccount(*olmAccount);
        !outcome.has_value()) {
        // A new account has been created
        auto job = q->callApi<UploadKeysJob>(olmAccount->deviceKeys());
        connect(job, &BaseJob::failure, q, [job]{
            qCWarning(E2EE) << "Failed to upload device keys:" << job->errorString();
        });
        connect(job, &BaseJob::success, q, [this]{
            trackedUsers += q->userId();
            outdatedUsers += q->userId();
            encryptionUpdateRequired = true;
        });
    } else { // account already existing
        if (outcome != OLM_SUCCESS)
            qCritical(E2EE)
                << "Could not unpickle Olm account, E2EE won't be available";
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
        q->setObjectName(userId % u"(?)");
        connectFn();
        return;
    }
    // Not good to go, try to ascertain the homeserver URL and flows
    if (userId.startsWith(u'@') && userId.indexOf(u':') != -1) {
        q->setObjectName(userId % u"(?)");
        q->resolveServer(userId);
        if (flow)
            connectSingleShot(q, &Connection::loginFlowsChanged, q,
                [this, flow, connectFn] {
                    if (loginFlows.contains(*flow))
                        connectFn();
                    else
                        emit q->loginError(
                            tr("Unsupported login flow"),
                            tr("The homeserver at %1 does not support"
                               " the login flow '%2'")
                                .arg(data->baseUrl().toDisplayString(),
                                     flow->type));
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
            SettingsGroup("Accounts"_ls).remove(userId());
            d->dropAccessToken();
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
#ifdef Quotient_E2EE_ENABLED
    d->oneTimeKeysCount = data.deviceOneTimeKeysCount();
    if (d->oneTimeKeysCount[SignedCurve25519Key] < 0.4 * d->olmAccount->maxNumberOfOneTimeKeys()
        && !d->isUploadingKeys) {
        d->isUploadingKeys = true;
        d->olmAccount->generateOneTimeKeys(
            d->olmAccount->maxNumberOfOneTimeKeys() / 2 - d->oneTimeKeysCount[SignedCurve25519Key]);
        auto keys = d->olmAccount->oneTimeKeys();
        auto job = d->olmAccount->createUploadKeyRequest(keys);
        run(job, ForegroundRequest);
        connect(job, &BaseJob::success, this,
                [this] { d->olmAccount->markKeysAsPublished(); });
        connect(job, &BaseJob::result, this,
                [this] { d->isUploadingKeys = false; });
    }
    if(d->firstSync) {
        d->loadDevicesList();
        d->firstSync = false;
    }

    d->consumeDevicesList(data.takeDevicesList());
#endif // Quotient_E2EE_ENABLED
    d->consumeToDeviceEvents(data.takeToDeviceEvents());
    d->data->setLastEvent(data.nextBatch());
    d->consumeRoomData(data.takeRoomData(), fromCache);
    d->consumeAccountData(data.takeAccountData());
    d->consumePresenceData(data.takePresenceData());
#ifdef Quotient_E2EE_ENABLED
    if(d->encryptionUpdateRequired) {
        d->loadOutdatedUserDevices();
        d->encryptionUpdateRequired = false;
    }
#endif
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
                           << terse << roomData.joinState
                           << "state - suspiciously fast turnaround";
        }
        if (auto* r = q->provideRoom(roomData.roomId, roomData.joinState)) {
            pendingStateRoomIds.removeOne(roomData.roomId);
            // Update rooms one by one, giving time to update the UI.
            QMetaObject::invokeMethod(
                r,
                [r, rd = std::move(roomData), fromCache] () mutable {
                    r->updateData(std::move(rd), fromCache);
                },
                Qt::QueuedConnection);
        }
    }
}

void Connection::Private::consumeAccountData(Events&& accountDataEvents)
{
    // After running this loop, the account data events not saved in
    // accountData (see the end of the loop body) are auto-cleaned away
    for (auto&& eventPtr: accountDataEvents) {
        switchOnType(*eventPtr,
            [this](const DirectChatEvent& dce) {
                // https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
                const auto& usersToDCs = dce.usersToDirectChats();
                DirectChatsMap remoteRemovals =
                    remove_if(directChats, [&usersToDCs, this](auto it) {
                        return !(
                            usersToDCs.contains(it.key()->id(), it.value())
                            || dcLocalAdditions.contains(it.key(), it.value()));
                    });
                remove_if(directChatUsers, [&remoteRemovals](auto it) {
                    return remoteRemovals.contains(it.value(), it.key());
                });
                // Remove from dcLocalRemovals what the server already has.
                remove_if(dcLocalRemovals, [&remoteRemovals](auto it) {
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
                remove_if(dcLocalAdditions, [&remoteAdditions](auto it) {
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
                        << QStringList(q->ignoredUsers().values()).join(u',');

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
    if (!toDeviceEvents.empty()) {
        qCDebug(E2EE) << "Consuming" << toDeviceEvents.size()
                      << "to-device events";
        for (auto&& tdEvt : toDeviceEvents) {
            if (processIfVerificationEvent(*tdEvt, false))
                continue;
            if (auto&& event = eventCast<EncryptedEvent>(std::move(tdEvt))) {
                if (event->algorithm() != OlmV1Curve25519AesSha2AlgoKey) {
                    qCDebug(E2EE) << "Unsupported algorithm" << event->id()
                                  << "for event" << event->algorithm();
                    continue;
                }
                if (isKnownCurveKey(event->senderId(), event->senderKey())) {
                    handleEncryptedToDeviceEvent(*event);
                    continue;
                }
                trackedUsers += event->senderId();
                outdatedUsers += event->senderId();
                encryptionUpdateRequired = true;
                pendingEncryptedEvents.push_back(std::move(event));
            }
        }
    }
#endif
}

#ifdef Quotient_E2EE_ENABLED
bool Connection::Private::processIfVerificationEvent(const Event& evt,
                                                     bool encrypted)
{
    return switchOnType(evt,
        [this, encrypted](const KeyVerificationRequestEvent& reqEvt) {
            setupKeyVerificationSession(reqEvt.fullJson()["sender"_ls].toString(),
                                        reqEvt, q, encrypted);
            return true;
        },
        [](const KeyVerificationDoneEvent&) { return true; },
        [this](const KeyVerificationEvent& kvEvt) {
            if (auto* const session =
                verificationSessions.value(kvEvt.transactionId())) {
                session->handleEvent(kvEvt);
                emit q->keyVerificationStateChanged(session, session->state());
            }
            return true;
        },
        false);
}

void Connection::Private::handleEncryptedToDeviceEvent(const EncryptedEvent& event)
{
    const auto [decryptedEvent, olmSessionId] = sessionDecryptMessage(event);
    if(!decryptedEvent) {
        qCWarning(E2EE) << "Failed to decrypt event" << event.id();
        return;
    }

    if (processIfVerificationEvent(*decryptedEvent, true))
        return;
    switchOnType(*decryptedEvent,
        [this, &event,
         olmSessionId = olmSessionId](const RoomKeyEvent& roomKeyEvent) {
            if (auto* detectedRoom = q->room(roomKeyEvent.roomId())) {
                detectedRoom->handleRoomKeyEvent(roomKeyEvent, event.senderId(),
                                                 olmSessionId);
            } else {
                qCDebug(E2EE)
                    << "Encrypted event room id" << roomKeyEvent.roomId()
                    << "is not found at the connection" << q->objectName();
            }
        },
        [](const Event& evt) {
            qCWarning(E2EE) << "Skipping encrypted to_device event, type"
                            << evt.matrixType();
        });
}
#endif

void Connection::Private::consumeDevicesList(DevicesList&& devicesList)
{
#ifdef Quotient_E2EE_ENABLED
    bool hasNewOutdatedUser = false;
    for(const auto &changed : devicesList.changed) {
        if(trackedUsers.contains(changed)) {
            outdatedUsers += changed;
            hasNewOutdatedUser = true;
        }
    }
    for(const auto &left : devicesList.left) {
        trackedUsers -= left;
        outdatedUsers -= left;
        deviceKeys.remove(left);
    }
    if(hasNewOutdatedUser) {
        loadOutdatedUserDevices();
    }
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

JoinRoomJob* Connection::joinRoom(const QString& roomAlias,
                                  const QStringList& serverNames)
{
    auto* const job = callApi<JoinRoomJob>(roomAlias, serverNames);
    // Upon completion, ensure a room object is created in case it hasn't come
    // with a sync yet. If the room object is not there, provideRoom() will
    // create it in Join state. finished() is used here instead of success()
    // to overtake clients that may add their own slots to finished().
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
    auto idParts = mediaId.split(u'/');
    Q_ASSERT_X(idParts.size() == 2, __FUNCTION__,
               qPrintable("'"_ls % mediaId
                          % "' doesn't look like 'serverName/localMediaId'"_ls));
    return idParts;
}

QUrl Connection::makeMediaUrl(QUrl mxcUrl) const
{
    Q_ASSERT(mxcUrl.scheme() == "mxc"_ls);
    QUrlQuery q(mxcUrl.query());
    q.addQueryItem(QStringLiteral("user_id"), userId());
    mxcUrl.setQuery(q);
    return mxcUrl;
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

#ifdef Quotient_E2EE_ENABLED
DownloadFileJob* Connection::downloadFile(
    const QUrl& url, const EncryptedFileMetadata& fileMetadata,
    const QString& localFilename)
{
    auto mediaId = url.authority() + url.path();
    auto idParts = splitMediaId(mediaId);
    return callApi<DownloadFileJob>(idParts.front(), idParts.back(),
                                    fileMetadata, localFilename);
}
#endif

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
                        || leaveJob->error() == BaseJob::NotFound) {
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
    } else
        run(forgetJob);
    connect(forgetJob, &BaseJob::result, this, [this, id, forgetJob] {
        // Leave room in case of success, or room not known by server
        if (forgetJob->error() == BaseJob::Success
            || forgetJob->error() == BaseJob::NotFound)
            d->removeRoom(id); // Delete the room from roomMap
        else
            qCWarning(MAIN).nospace() << "Error forgetting room " << id << ": "
                                      << forgetJob->errorString();
    });
    return forgetJob;
}

SendToDeviceJob* Connection::sendToDevices(
    const QString& eventType, const UsersToDevicesToContent& contents)
{
    return callApi<SendToDeviceJob>(BackgroundRequest, eventType,
                                    QString::fromLatin1(generateTxnId()), contents);
}

SendMessageJob* Connection::sendMessage(const QString& roomId,
                                        const RoomEvent& event)
{
    const auto txnId = event.transactionId().isEmpty() ? generateTxnId()
                                                       : event.transactionId().toLatin1();
    return callApi<SendMessageJob>(roomId, event.matrixType(), QString::fromLatin1(txnId),
                                   event.contentJson());
}

QUrl Connection::homeserver() const { return d->data->baseUrl(); }

QString Connection::domain() const { return userId().section(u':', 1); }

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

bool Connection::roomSucceeds(const QString& maybePredecessorId,
                              const QString& maybeSuccessorId) const
{
    static constexpr auto AnyJoinStateMask = JoinState::Invite | JoinState::Join
                                             | JoinState::Knock
                                             | JoinState::Leave;

    for (auto r = room(maybePredecessorId, AnyJoinStateMask); r != nullptr;) {
        const auto& currentSuccId = r->successorId(); // Search forward
        if (currentSuccId.isEmpty())
            break;
        if (currentSuccId == maybeSuccessorId)
            return true;
        r = room(currentSuccId, AnyJoinStateMask);
    }
    for (auto r = room(maybeSuccessorId, AnyJoinStateMask); r != nullptr;) {
        const auto& currentPredId = r->predecessorId(); // Search backward
        if (currentPredId.isEmpty())
            break;
        if (currentPredId == maybePredecessorId)
            return true;
        r = room(currentPredId, AnyJoinStateMask);
    }
    return false; // Can't ascertain succession
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
            else if (roomSucceeds(roomId, mappedId)) {
                qCDebug(MAIN) << "Not remapping alias" << a << "from"
                              << mappedId << "to predecessor" << roomId;
                continue;
            } else if (roomSucceeds(mappedId, roomId))
                qCDebug(MAIN) << "Remapping alias" << a << "from" << mappedId
                              << "to successor" << roomId;
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
    if (const auto v = d->userMap.value(uId, nullptr))
        return v;
    // Before creating a user object, check that the user id is well-formed
    // (it's faster to just do a lookup above before validation)
    if (!uId.startsWith(u'@') || serverPart(uId).isEmpty()) {
        qCCritical(MAIN) << "Malformed userId:" << uId;
        return nullptr;
    }
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
    return d->olmAccount.get();
}
#endif // Quotient_E2EE_ENABLED

SyncJob* Connection::syncJob() const { return d->syncJob; }

int Connection::millisToReconnect() const
{
    return d->syncJob ? d->syncJob->millisToRetry() : 0;
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
            qCDebug(MAIN) << "Room" << r->objectName() << "in state" << terse
                          << r->joinState() << "will be deleted";
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
        removals = remove_if(d->directChats,
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

bool Connection::isIgnored(const QString& userId) const
{
    return ignoredUsers().contains(userId);
}

bool Connection::isIgnored(const User* user) const
{
    Q_ASSERT(user != nullptr);
    return isIgnored(user->id());
}

IgnoredUsersList Connection::ignoredUsers() const
{
    const auto* event = accountData<IgnoredUsersEvent>();
    return event ? event->ignoredUsers() : IgnoredUsersList();
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
    const std::pair roomKey { id, joinState == JoinState::Invite };
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
        // No Invite either, setup a new room object in Join state
        joinState = JoinState::Join;
    }

    if (!room) {
        Q_ASSERT(joinState.has_value());
        room = roomFactory()(this, id, *joinState);
        if (!room) {
            qCCritical(MAIN) << "Failed to create a room" << id;
            return nullptr;
        }
        d->roomMap.insert(roomKey, room);
        connect(room, &Room::beforeDestruction, this,
                &Connection::aboutToDeleteRoom);
        connect(room, &Room::baseStateLoaded, this, [this, room] {
            emit loadedRoomState(room);
            if (d->capabilities.roomVersions)
                room->checkVersion();
            // Otherwise, the version will be checked in reloadCapabilities()
        });
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

room_factory_t Connection::_roomFactory = defaultRoomFactory<>;
user_factory_t Connection::_userFactory = defaultUserFactory<>;

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
        const auto data =
            d->cacheToBinary
                ? QCborValue::fromJsonValue(r->toJson()).toCbor()
                : QJsonDocument(r->toJson()).toJson(QJsonDocument::Compact);
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
            Event::basicJson(DirectChatEvent::TypeId, toJson(d->directChats))
        };
        for (const auto& e : d->accountData)
            accountDataEvents.append(
                Event::basicJson(e.first, e.second->contentJson()));

        rootObj.insert(QStringLiteral("account_data"),
                       QJsonObject {
                           { QStringLiteral("events"), accountDataEvents } });
    }
#ifdef Quotient_E2EE_ENABLED
    {
        QJsonObject keysJson = toJson(d->oneTimeKeysCount);
        rootObj.insert(QStringLiteral("device_one_time_keys_count"), keysJson);
    }
#endif

    const auto data =
        d->cacheToBinary ? QCborValue::fromJsonValue(rootObj).toCbor()
                         : QJsonDocument(rootObj).toJson(QJsonDocument::Compact);
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
    return stateCacheDir().path() % u'/';
}

QDir Connection::stateCacheDir() const
{
    auto safeUserId = userId();
    safeUserId.replace(u':', u'_');
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
            [this,job] { emit turnServersChanged(job->data()); });
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

#ifdef Quotient_E2EE_ENABLED
bool Connection::isQueryingKeys() const
{
    return d->currentQueryKeysJob != nullptr;
}

void Connection::Private::handleQueryKeys(const QueryKeysJob* job)
{
    const auto newDeviceKeys = job->deviceKeys();
    for (const auto& [user, keys] : asKeyValueRange(newDeviceKeys)) {
        QHash<QString, Quotient::DeviceKeys> oldDevices = deviceKeys[user];
        deviceKeys[user].clear();
        for(const auto &device : keys) {
            if(device.userId != user) {
                qWarning(E2EE)
                    << "mxId mismatch during device key verification:"
                    << device.userId << user;
                continue;
            }
            if (!std::all_of(device.algorithms.cbegin(),
                            device.algorithms.cend(),
                            isSupportedAlgorithm)) {
                qWarning(E2EE) << "Unsupported encryption algorithms found"
                               << device.algorithms;
                continue;
            }
            if (!verifyIdentitySignature(device, device.deviceId,
                                        device.userId)) {
                qWarning(E2EE) << "Failed to verify devicekeys signature. "
                                  "Skipping this device";
                continue;
            }
            if (oldDevices.contains(device.deviceId)) {
                if (oldDevices[device.deviceId].keys["ed25519:"_ls % device.deviceId]
                    != device.keys["ed25519:"_ls % device.deviceId]) {
                    qDebug(E2EE)
                        << "Device reuse detected. Skipping this device";
                    continue;
                }
            }
            deviceKeys[user][device.deviceId] = SLICE(device, DeviceKeys);
        }
        outdatedUsers -= user;
    }
    saveDevicesList();

    // A completely faithful code would call std::partition() with bare
    // isKnownCurveKey(), then handleEncryptedToDeviceEvent() on each event
    // with the known key, and then std::erase()... but
    // handleEncryptedToDeviceEvent() doesn't have side effects on the handled
    // events so a small corner-cutting should be fine.
    std::erase_if(pendingEncryptedEvents,
                  [this](const event_ptr_tt<EncryptedEvent>& pendingEvent) {
                      if (!isKnownCurveKey(
                              pendingEvent->fullJson()[SenderKey].toString(),
                              pendingEvent->contentPart<QString>(SenderKeyKeyL)))
                          return false;
                      handleEncryptedToDeviceEvent(*pendingEvent);
                      return true;
                  });
}

void Connection::Private::loadOutdatedUserDevices()
{
    QHash<QString, QStringList> users;
    for(const auto &user : outdatedUsers) {
        users[user] += QStringList();
    }
    if(currentQueryKeysJob) {
        currentQueryKeysJob->abandon();
        currentQueryKeysJob = nullptr;
    }
    auto queryKeysJob = q->callApi<QueryKeysJob>(users);
    currentQueryKeysJob = queryKeysJob;
    connect(queryKeysJob, &BaseJob::success, q, [this, queryKeysJob](){
        currentQueryKeysJob = nullptr;
        if (queryKeysJob->error() == BaseJob::Success) {
            handleQueryKeys(queryKeysJob);
        }
        emit q->finishedQueryingKeys();
    });
}

void Connection::Private::saveDevicesList()
{
    q->database()->transaction();
    auto query = q->database()->prepareQuery(
        QStringLiteral("DELETE FROM tracked_users"));
    q->database()->execute(query);
    query.prepare(QStringLiteral(
        "INSERT INTO tracked_users(matrixId) VALUES(:matrixId);"));
    for (const auto& user : trackedUsers) {
        query.bindValue(":matrixId"_ls, user);
        q->database()->execute(query);
    }

    query.prepare(QStringLiteral("DELETE FROM outdated_users"));
    q->database()->execute(query);
    query.prepare(QStringLiteral(
        "INSERT INTO outdated_users(matrixId) VALUES(:matrixId);"));
    for (const auto& user : outdatedUsers) {
        query.bindValue(":matrixId"_ls, user);
        q->database()->execute(query);
    }

    query.prepare(QStringLiteral(
        "INSERT INTO tracked_devices"
        "(matrixId, deviceId, curveKeyId, curveKey, edKeyId, edKey, verified) "
        "SELECT :matrixId, :deviceId, :curveKeyId, :curveKey, :edKeyId, :edKey, :verified WHERE NOT EXISTS(SELECT 1 FROM tracked_devices WHERE matrixId=:matrixId AND deviceId=:deviceId);"
        ));
    for (const auto& [user, devices] : asKeyValueRange(deviceKeys)) {
        for (const auto& device : devices) {
            auto keys = device.keys.keys();
            auto curveKeyId = keys[0].startsWith("curve"_ls) ? keys[0] : keys[1];
            auto edKeyId = keys[0].startsWith("ed"_ls) ? keys[0] : keys[1];

            query.bindValue(":matrixId"_ls, user);
            query.bindValue(":deviceId"_ls, device.deviceId);
            query.bindValue(":curveKeyId"_ls, curveKeyId);
            query.bindValue(":curveKey"_ls, device.keys[curveKeyId]);
            query.bindValue(":edKeyId"_ls, edKeyId);
            query.bindValue(":edKey"_ls, device.keys[edKeyId]);
            // If the device gets saved here, it can't be verified
            query.bindValue(":verified"_ls, false);

            q->database()->execute(query);
        }
    }
    q->database()->commit();
}

void Connection::Private::loadDevicesList()
{
    auto query = q->database()->prepareQuery(QStringLiteral("SELECT * FROM tracked_users;"));
    q->database()->execute(query);
    while(query.next()) {
        trackedUsers += query.value(0).toString();
    }

    query = q->database()->prepareQuery(QStringLiteral("SELECT * FROM outdated_users;"));
    q->database()->execute(query);
    while(query.next()) {
        outdatedUsers += query.value(0).toString();
    }

    query = q->database()->prepareQuery(QStringLiteral("SELECT * FROM tracked_devices;"));
    q->database()->execute(query);
    while(query.next()) {
        deviceKeys[query.value("matrixId"_ls).toString()][query.value("deviceId"_ls).toString()] = DeviceKeys {
            query.value("matrixId"_ls).toString(),
            query.value("deviceId"_ls).toString(),
            { "m.olm.v1.curve25519-aes-sha2"_ls, "m.megolm.v1.aes-sha2"_ls},
            {{query.value("curveKeyId"_ls).toString(), query.value("curveKey"_ls).toString()},
             {query.value("edKeyId"_ls).toString(), query.value("edKey"_ls).toString()}},
             {} // Signatures are not saved/loaded as they are not needed after initial validation
        };
    }

}

void Connection::encryptionUpdate(const Room* room, const QList<User*>& invited)
{
    for (const auto& user : room->users() + invited) {
        if(!d->trackedUsers.contains(user->id())) {
            d->trackedUsers += user->id();
            d->outdatedUsers += user->id();
            d->encryptionUpdateRequired = true;
        }
    }
}

void Connection::Private::saveOlmAccount()
{
    qCDebug(E2EE) << "Saving olm account";
    database->storeOlmAccount(*olmAccount);
}

QJsonObject Connection::decryptNotification(const QJsonObject& notification)
{
    if (auto r = room(notification["room_id"_ls].toString()))
        if (auto event =
                loadEvent<EncryptedEvent>(notification["event"_ls].toObject()))
            if (const auto decrypted = r->decryptMessage(*event))
                return decrypted->fullJson();
    return {};
}

Database* Connection::database() const
{
    return d->database;
}

UnorderedMap<QString, QOlmInboundGroupSession>
Connection::loadRoomMegolmSessions(const Room* room) const
{
    return database()->loadMegolmSessions(room->id());
}

void Connection::saveMegolmSession(const Room* room,
                                   const QOlmInboundGroupSession& session) const
{
    database()->saveMegolmSession(room->id(), session);
}

QStringList Connection::devicesForUser(const QString& userId) const
{
    return d->deviceKeys.value(userId).keys();
}

QString Connection::Private::curveKeyForUserDevice(const QString& userId,
                                                   const QString& device) const
{
    return deviceKeys[userId][device].keys["curve25519:"_ls % device];
}

QString Connection::edKeyForUserDevice(const QString& userId,
                                       const QString& deviceId) const
{
    return d->deviceKeys[userId][deviceId].keys["ed25519:"_ls % deviceId];
}

bool Connection::Private::isKnownCurveKey(const QString& userId,
                                          const QString& curveKey) const
{
    auto query = database->prepareQuery(
        QStringLiteral("SELECT * FROM tracked_devices WHERE matrixId=:matrixId "
                       "AND curveKey=:curveKey"));
    query.bindValue(":matrixId"_ls, userId);
    query.bindValue(":curveKey"_ls, curveKey);
    database->execute(query);
    return query.next();
}

bool Connection::hasOlmSession(const QString& user,
                               const QString& deviceId) const
{
    const auto& curveKey = d->curveKeyForUserDevice(user, deviceId);
    return d->olmSessions.contains(curveKey) && !d->olmSessions[curveKey].empty();
}

std::pair<QOlmMessage::Type, QByteArray> Connection::Private::olmEncryptMessage(
    const QString& userId, const QString& device,
    const QByteArray& message) const
{
    const auto& curveKey = curveKeyForUserDevice(userId, device);
    const auto& olmSession = olmSessions.at(curveKey).front();
    const auto result = olmSession.encrypt(message);
    database->updateOlmSession(curveKey, olmSession);
    return { result.type(), result.toCiphertext() };
}

bool Connection::Private::createOlmSession(const QString& targetUserId,
                                           const QString& targetDeviceId,
                                           const OneTimeKeys& oneTimeKeyObject)
{
    static QOlmUtility verifier;
    qDebug(E2EE) << "Creating a new session for" << targetUserId
                 << targetDeviceId;
    if (oneTimeKeyObject.isEmpty()) {
        qWarning(E2EE) << "No one time key for" << targetUserId
                       << targetDeviceId;
        return false;
    }
    auto* signedOneTimeKey =
        std::get_if<SignedOneTimeKey>(&*oneTimeKeyObject.begin());
    if (!signedOneTimeKey) {
        qWarning(E2EE) << "No signed one time key for" << targetUserId
                       << targetDeviceId;
        return false;
    }
    // Verify contents of signedOneTimeKey - for that, drop `signatures` and
    // `unsigned` and then verify the object against the respective signature
    const auto signature =
        signedOneTimeKey->signature(targetUserId, targetDeviceId);
    if (!verifier.ed25519Verify(
            q->edKeyForUserDevice(targetUserId, targetDeviceId).toLatin1(),
            signedOneTimeKey->toJsonForVerification(),
            signature)) {
        qWarning(E2EE) << "Failed to verify one-time-key signature for"
                       << targetUserId << targetDeviceId
                       << ". Skipping this device.";
        return false;
    }
    const auto recipientCurveKey =
        curveKeyForUserDevice(targetUserId, targetDeviceId).toLatin1();
    auto session = olmAccount->createOutboundSession(recipientCurveKey,
                                                     signedOneTimeKey->key());
    if (!session) {
        qCWarning(E2EE) << "Failed to create olm session for "
                        << recipientCurveKey << session.error();
        return false;
    }
    saveSession(*session, QString::fromLatin1(recipientCurveKey));
    olmSessions[QString::fromLatin1(recipientCurveKey)].push_back(std::move(*session));
    return true;
}

QJsonObject Connection::Private::assembleEncryptedContent(
    QJsonObject payloadJson, const QString& targetUserId,
    const QString& targetDeviceId) const
{
    payloadJson.insert(SenderKey, data->userId());
//    eventJson.insert("sender_device"_ls, data->deviceId());
    payloadJson.insert("keys"_ls,
                       QJsonObject{
                           { Ed25519Key,
                             QString::fromLatin1(olmAccount->identityKeys().ed25519) } });
    payloadJson.insert("recipient"_ls, targetUserId);
    payloadJson.insert(
        "recipient_keys"_ls,
        QJsonObject{ { Ed25519Key,
                       q->edKeyForUserDevice(targetUserId, targetDeviceId) } });
    const auto [type, cipherText] = olmEncryptMessage(
        targetUserId, targetDeviceId,
        QJsonDocument(payloadJson).toJson(QJsonDocument::Compact));
    QJsonObject encrypted {
        { curveKeyForUserDevice(targetUserId, targetDeviceId),
          QJsonObject { { "type"_ls, type },
                        { "body"_ls, QString::fromLatin1(cipherText) } } }
    };
    return EncryptedEvent(encrypted, QString::fromLatin1(olmAccount->identityKeys().curve25519))
        .contentJson();
}

std::pair<EventPtr, QString> Connection::Private::sessionDecryptMessage(
    const EncryptedEvent& encryptedEvent)
{
#ifndef Quotient_E2EE_ENABLED
    qWarning(E2EE) << "End-to-end encryption (E2EE) support is turned off.";
        return {};
#else
    if (encryptedEvent.algorithm() != OlmV1Curve25519AesSha2AlgoKey)
        return {};

    const auto identityKey = olmAccount->identityKeys().curve25519;
    const auto personalCipherObject =
        encryptedEvent.ciphertext(QString::fromLatin1(identityKey));
    if (personalCipherObject.isEmpty()) {
        qDebug(E2EE) << "Encrypted event is not for the current device";
        return {};
    }
    const auto [decrypted, olmSessionId] =
        sessionDecryptMessage(personalCipherObject,
                              encryptedEvent.senderKey().toLatin1());
    if (decrypted.isEmpty()) {
        qDebug(E2EE) << "Problem with new session from senderKey:"
                     << encryptedEvent.senderKey()
                     << olmAccount->oneTimeKeys().keys;

        auto query = database->prepareQuery("SELECT deviceId FROM tracked_devices WHERE curveKey=:curveKey;"_ls);
        query.bindValue(":curveKey"_ls, encryptedEvent.senderKey());
        database->execute(query);
        if (!query.next()) {
            qCWarning(E2EE) << "Unknown device while trying to recover from broken olm session";
            return {};
        }
        auto senderId = encryptedEvent.senderId();
        auto deviceId = query.value("deviceId"_ls).toString();
        QHash<QString, QHash<QString, QString>> hash{
            { encryptedEvent.senderId(), { { deviceId, "signed_curve25519"_ls } } }
        };
        auto job = q->callApi<ClaimKeysJob>(hash);
        connect(job, &BaseJob::finished, q, [this, deviceId, job, senderId] {
            if (triedDevices.contains({senderId, deviceId})) {
                return;
            }
            triedDevices += {senderId, deviceId};
            qDebug(E2EE) << "Sending dummy event to" << senderId << deviceId;
            createOlmSession(senderId, deviceId, job->oneTimeKeys()[senderId][deviceId]);
            q->sendToDevice(senderId, deviceId, DummyEvent(), true);
        });
        return {};
    }

    auto&& decryptedEvent =
        fromJson<EventPtr>(QJsonDocument::fromJson(decrypted.toUtf8()));

    if (auto sender = decryptedEvent->fullJson()[SenderKey].toString();
        sender != encryptedEvent.senderId()) {
        qWarning(E2EE) << "Found user" << sender << "instead of sender"
                       << encryptedEvent.senderId() << "in Olm plaintext";
        return {};
    }

    auto query = database->prepareQuery(QStringLiteral("SELECT edKey FROM tracked_devices WHERE curveKey=:curveKey;"));
    query.bindValue(":curveKey"_ls, encryptedEvent.contentJson()["sender_key"_ls].toString());
    database->execute(query);
    if (!query.next()) {
        qWarning(E2EE)
            << "Received olm message from unknown device"
            << encryptedEvent.contentJson()["sender_key"_ls].toString();
        return {};
    }
    auto edKey = decryptedEvent->fullJson()["keys"_ls]["ed25519"_ls].toString();
    if (edKey.isEmpty() || query.value(QStringLiteral("edKey")).toString() != edKey) {
        qDebug(E2EE) << "Received olm message with invalid ed key";
        return {};
    }

    // TODO: keys to constants
    const auto decryptedEventObject = decryptedEvent->fullJson();
    const auto recipient = decryptedEventObject.value("recipient"_ls).toString();
    if (recipient != data->userId()) {
        qDebug(E2EE) << "Found user" << recipient << "instead of us"
                     << data->userId() << "in Olm plaintext";
        return {};
    }
    const auto ourKey = decryptedEventObject.value("recipient_keys"_ls).toObject()
        .value(Ed25519Key).toString();
    if (ourKey != QString::fromUtf8(olmAccount->identityKeys().ed25519)) {
        qDebug(E2EE) << "Found key" << ourKey
                     << "instead of ours own ed25519 key"
                     << olmAccount->identityKeys().ed25519
                     << "in Olm plaintext";
        return {};
    }

    return { std::move(decryptedEvent), olmSessionId };
#endif // Quotient_E2EE_ENABLED
}

std::pair<QString, QString> Connection::Private::sessionDecryptMessage(
    const QJsonObject& personalCipherObject, const QByteArray& senderKey)
{
    const auto msgType = static_cast<QOlmMessage::Type>(
        personalCipherObject.value(TypeKey).toInt(-1));
    if (msgType != QOlmMessage::General && msgType != QOlmMessage::PreKey) {
        qCWarning(E2EE) << "Olm message has incorrect type" << msgType;
        return {};
    }
    QOlmMessage message {
        personalCipherObject.value(BodyKey).toString().toLatin1(), msgType
    };
    for (const auto& session : olmSessions[QString::fromLatin1(senderKey)])
        if (msgType == QOlmMessage::General
            || session.matchesInboundSessionFrom(QString::fromLatin1(senderKey), message)) {
            return doDecryptMessage(session, message, [this, &session] {
                q->database()->setOlmSessionLastReceived(
                    session.sessionId(), QDateTime::currentDateTime());
            });
        }

    if (msgType == QOlmMessage::General) {
        qCWarning(E2EE) << "Failed to decrypt message";
        return {};
    }

    qCDebug(E2EE) << "Creating new inbound session"; // Pre-key messages only
    auto newSessionResult =
        olmAccount->createInboundSessionFrom(senderKey, message);
    if (!newSessionResult) {
        qCWarning(E2EE)
            << "Failed to create inbound session for" << senderKey
            << "with error" << newSessionResult.error();
        return {};
    }
    auto&& newSession = std::move(*newSessionResult);
    if (olmAccount->removeOneTimeKeys(newSession) != OLM_SUCCESS) {
        qWarning(E2EE) << "Failed to remove one time key for session"
                       << newSession.sessionId();
        // Keep going though
    }
    return doDecryptMessage(
        newSession, message, [this, &senderKey, &newSession] {
            saveSession(newSession, QString::fromLatin1(senderKey));
            olmSessions[QString::fromLatin1(senderKey)].push_back(std::move(newSession));
        });
}

void Connection::Private::loadSessions()
{
    olmSessions = q->database()->loadOlmSessions();
}

void Connection::Private::saveSession(const QOlmSession& session,
                                      const QString& senderKey) const
{
    q->database()->saveOlmSession(senderKey, session,
                                  QDateTime::currentDateTime());
}

template <typename FnT>
std::pair<QString, QString> Connection::Private::doDecryptMessage(
    const QOlmSession& session, const QOlmMessage& message, FnT&& andThen) const
{
    const auto expectedMessage = session.decrypt(message);
    if (expectedMessage) {
        const std::pair result { QString::fromUtf8(*expectedMessage), QString::fromUtf8(session.sessionId()) };
        andThen();
        return result;
    }
    const auto errorLine = message.type() == QOlmMessage::PreKey
                           ? "Failed to decrypt prekey message:"
                           : "Failed to decrypt message:";
    qCDebug(E2EE) << errorLine << expectedMessage.error();
    return {};
}

void Connection::sendSessionKeyToDevices(
    const QString& roomId, const QOlmOutboundGroupSession& outboundSession,
    const QMultiHash<QString, QString>& devices)
{
    const auto& sessionId = outboundSession.sessionId();
    const auto& sessionKey = outboundSession.sessionKey();
    const auto& index = outboundSession.sessionMessageIndex();
    qDebug(E2EE) << "Sending room key to devices:" << sessionId << index;
    QHash<QString, QHash<QString, QString>> hash;
    for (const auto& [userId, deviceId] : asKeyValueRange(devices))
        if (!hasOlmSession(userId, deviceId)) {
            hash[userId].insert(deviceId, "signed_curve25519"_ls);
            qDebug(E2EE) << "Adding" << userId << deviceId
                         << "to keys to claim";
        }

    const auto sendKey = [devices, this, sessionId, index, sessionKey, roomId] {
        QHash<QString, QHash<QString, QJsonObject>> usersToDevicesToContent;
        for (const auto& [targetUserId, targetDeviceId] : asKeyValueRange(devices)) {
            if (!hasOlmSession(targetUserId, targetDeviceId))
                continue;

            // Noisy and leaks the key to logs but nice for debugging
            // qDebug(E2EE) << "Creating the payload for" << targetUserId
            //              << targetDeviceId << sessionId << sessionKey.toHex();
            const auto keyEventJson = RoomKeyEvent(MegolmV1AesSha2AlgoKey,
                                                   roomId, QString::fromLatin1(sessionId),
                                                   QString::fromLatin1(sessionKey))
                                            .fullJson();

            usersToDevicesToContent[targetUserId][targetDeviceId] =
                d->assembleEncryptedContent(keyEventJson, targetUserId,
                                            targetDeviceId);
        }
        if (!usersToDevicesToContent.empty()) {
            sendToDevices(EncryptedEvent::TypeId, usersToDevicesToContent);
            QVector<std::tuple<QString, QString, QString>> receivedDevices;
            receivedDevices.reserve(devices.size());
            for (const auto& [user, device] : asKeyValueRange(devices))
                receivedDevices.push_back(
                    { user, device, d->curveKeyForUserDevice(user, device) });

            database()->setDevicesReceivedKey(roomId, receivedDevices,
                                              sessionId, index);
        }
    };

    if (hash.isEmpty()) {
        sendKey();
        return;
    }

    auto job = callApi<ClaimKeysJob>(hash);
    connect(job, &BaseJob::success, this, [job, this, sendKey] {
        for (const auto oneTimeKeys = job->oneTimeKeys();
             const auto& [userId, devices] : asKeyValueRange(oneTimeKeys)) {
            for (const auto& [deviceId, keys] : asKeyValueRange(devices)) {
                d->createOlmSession(userId, deviceId, keys);
            }
        }
        sendKey();
    });
}

Omittable<QOlmOutboundGroupSession> Connection::loadCurrentOutboundMegolmSession(
    const QString& roomId) const
{
    return d->database->loadCurrentOutboundMegolmSession(roomId);
}

void Connection::saveCurrentOutboundMegolmSession(
    const QString& roomId, const QOlmOutboundGroupSession& session) const
{
    d->database->saveCurrentOutboundMegolmSession(roomId, session);
}

template <typename... ArgTs>
KeyVerificationSession* Connection::Private::setupKeyVerificationSession(
    ArgTs&&... sessionArgs)
{
    auto session =
        new KeyVerificationSession(std::forward<ArgTs>(sessionArgs)...);
    verificationSessions.insert(session->transactionId(), session);
    connect(session, &QObject::destroyed, q,
            [this, txnId = session->transactionId()] {
                verificationSessions.remove(txnId);
            });
    emit q->newKeyVerificationSession(session);
    return session;
}

KeyVerificationSession* Connection::startKeyVerificationSession(
    const QString& userId, const QString& deviceId)
{
    return d->setupKeyVerificationSession(userId, deviceId, this);
}

void Connection::sendToDevice(const QString& targetUserId,
                              const QString& targetDeviceId, const Event& event,
                              bool encrypted)
{
    const auto contentJson =
        encrypted ? d->assembleEncryptedContent(event.fullJson(), targetUserId,
                                                targetDeviceId)
                  : event.contentJson();
    sendToDevices(encrypted ? EncryptedEvent::TypeId : event.type(),
                  { { targetUserId, { { targetDeviceId, contentJson } } } });
}

bool Connection::isVerifiedSession(const QByteArray& megolmSessionId) const
{
    auto query = database()->prepareQuery("SELECT olmSessionId FROM inbound_megolm_sessions WHERE sessionId=:sessionId;"_ls);
    query.bindValue(":sessionId"_ls, megolmSessionId);
    database()->execute(query);
    if (!query.next()) {
        return false;
    }
    auto olmSessionId = query.value("olmSessionId"_ls).toString();
    query.prepare("SELECT senderKey FROM olm_sessions WHERE sessionId=:sessionId;"_ls);
    query.bindValue(":sessionId"_ls, olmSessionId.toLatin1());
    database()->execute(query);
    if (!query.next()) {
        return false;
    }
    auto curveKey = query.value("senderKey"_ls).toString();
    query.prepare("SELECT verified FROM tracked_devices WHERE curveKey=:curveKey;"_ls);
    query.bindValue(":curveKey"_ls, curveKey);
    database()->execute(query);
    return query.next() && query.value("verified"_ls).toBool();
}

bool Connection::isVerifiedDevice(const QString& userId, const QString& deviceId) const
{
    auto query = database()->prepareQuery("SELECT verified FROM tracked_devices WHERE deviceId=:deviceId AND matrixId=:matrixId;"_ls);
    query.bindValue(":deviceId"_ls, deviceId);
    query.bindValue(":matrixId"_ls, userId);
    database()->execute(query);
    return query.next() && query.value("verified"_ls).toBool();
}

bool Connection::isKnownE2eeCapableDevice(const QString& userId, const QString& deviceId) const
{
    auto query = database()->prepareQuery("SELECT verified FROM tracked_devices WHERE deviceId=:deviceId AND matrixId=:matrixId;"_ls);
    query.bindValue(":deviceId"_ls, deviceId);
    query.bindValue(":matrixId"_ls, userId);
    database()->execute(query);
    return query.next();
}

#endif

Connection* Connection::makeMockConnection(const QString& mxId)
{
    auto* c = new Connection;
    c->d->completeSetup(mxId);
    return c;
}

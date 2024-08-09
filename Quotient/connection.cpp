// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2019 Ville Ranki <ville.ranki@iki.fi>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "connection.h"

#include "connection_p.h"
#include "connectiondata.h"
#include "connectionencryptiondata_p.h"
#include "database.h"
#include "logging_categories_p.h"
#include "qt_connection_util.h"
#include "room.h"
#include "settings.h"
#include "user.h"
#include "networkaccessmanager.h"

#include "csapi/account-data.h"
#include "csapi/joining.h"
#include "csapi/leaving.h"
#include "csapi/logout.h"
#include "csapi/room_send.h"
#include "csapi/to_device.h"
#include "csapi/voip.h"
#include "csapi/wellknown.h"
#include "csapi/whoami.h"

#include "e2ee/qolminboundsession.h"

#include "events/directchatevent.h"
#include "events/encryptionevent.h"
#include "jobs/downloadfilejob.h"
#include "jobs/mediathumbnailjob.h"
#include "jobs/syncjob.h"

// moc needs fully defined deps, see https://www.qt.io/blog/whats-new-in-qmetatype-qvariant
#include "moc_connection.cpp" // NOLINT(bugprone-suspicious-include)

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QMimeDatabase>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QStringBuilder>
#include <QtCore/QRandomGenerator>
#include <QtCore/QTimer>
#include <QtNetwork/QDnsLookup>
#include <QtNetwork/QTcpServer>
#include <qt6keychain/keychain.h>

using namespace Quotient;
using namespace Qt::Literals::StringLiterals;

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
    //connect(qApp, &QCoreApplication::aboutToQuit, this, &Connection::saveOlmAccount);
    d->q = this; // All d initialization should occur before this line
    setObjectName(server.toString());
}

Connection::Connection(QObject* parent) : Connection({}, parent) {}

Connection::~Connection()
{
    qCDebug(MAIN) << "deconstructing connection object for" << userId();
    stopSync();
}

void Connection::resolveServer(const QString& mxid)
{
    d->resolverJob.abandon(); // The previous network request is no more relevant

    auto maybeBaseUrl = QUrl::fromUserInput(mxid.startsWith(u'@') ? serverPart(mxid) : mxid);
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
    // Make sure baseUrl is restored in any case, even an abandon, and before any further processing
    connect(d->resolverJob.get(), &BaseJob::finished, this,
            [this, oldBaseUrl] { d->data->setBaseUrl(oldBaseUrl); });
    d->resolverJob.onResult(this, [this, maybeBaseUrl]() mutable {
        if (d->resolverJob->error() != BaseJob::NotFound) {
            if (!d->resolverJob->status().good()) {
                qCWarning(MAIN) << "Fetching .well-known file failed, FAIL_PROMPT";
                emit resolveError(tr("Failed resolving the homeserver"));
                return;
            }
            const QUrl baseUrl{ d->resolverJob->data().homeserver.baseUrl };
            if (baseUrl.isEmpty()) {
                qCWarning(MAIN) << "base_url not provided, FAIL_PROMPT";
                emit resolveError(tr("The homeserver base URL is not provided"));
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
            qCInfo(MAIN) << "No .well-known file, using" << maybeBaseUrl << "for base URL";
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
    d->ensureHomeserver(userId, LoginFlows::Password).then([=, this] {
        d->loginToServer(LoginFlows::Password.type, makeUserIdentifier(userId),
                         password, /*token*/ QString(), deviceId, initialDeviceName);
    });
}

SsoSession* Connection::prepareForSso(const QString& initialDeviceName,
                                      const QString& deviceId)
{
    return new SsoSession(this, initialDeviceName, deviceId);
}

void Connection::loginWithToken(const QString& loginToken,
                                const QString& initialDeviceName,
                                const QString& deviceId)
{
    Q_ASSERT(d->data->baseUrl().isValid() && d->loginFlows.contains(LoginFlows::Token));
    d->loginToServer(LoginFlows::Token.type, std::nullopt /*user is encoded in loginToken*/,
                     QString() /*password*/, loginToken, deviceId, initialDeviceName);
}

void Connection::assumeIdentity(const QString& mxId, const QString& accessToken)
{
    d->ensureHomeserver(mxId).then([this, mxId, accessToken] {
        d->data->setToken(accessToken.toLatin1());
        callApi<GetTokenOwnerJob>().onResult([this, mxId](const GetTokenOwnerJob* job) {
            switch (job->error()) {
            case BaseJob::Success:
                if (mxId != job->userId())
                    qCWarning(MAIN).nospace()
                        << "The access_token owner (" << job->userId()
                        << ") is different from passed MXID (" << mxId << ")!";
                d->data->setDeviceId(job->deviceId());
                d->completeSetup(job->userId());
                return;
            case BaseJob::NetworkError:
                emit networkError(job->errorString(), job->rawDataSample(), job->maxRetries(), -1);
                return;
            default: emit loginError(job->errorString(), job->rawDataSample());
            }
        });
    });
}

void Connection::assumeOidcIdentity(const QString& mxId, const QString& refreshToken, const QString& clientId, const QString& tokenEndpoint)
{
    qWarning() << mxId << refreshToken << clientId << tokenEndpoint;
    d->ensureHomeserver(mxId).then([this, mxId, refreshToken, clientId, tokenEndpoint] {
        d->data->setRefreshToken(refreshToken);
        d->data->setClientId(clientId);
        d->data->setTokenEndpoint(tokenEndpoint);
        this->refreshToken();
        connect(this, &Connection::refreshTokenChanged, this, [this, mxId](){
            qWarning() << d->data->refreshToken() << d->data->accessToken();
            callApi<GetTokenOwnerJob>().onResult([this, mxId](const GetTokenOwnerJob* job) {
                switch (job->error()) {
                case BaseJob::Success:
                    if (mxId != job->userId())
                        qCWarning(MAIN).nospace()
                            << "The access_token owner (" << job->userId()
                            << ") is different from passed MXID (" << mxId << ")!";
                    d->data->setDeviceId(job->deviceId());
                    d->completeSetup(job->userId());
                    return;
                case BaseJob::NetworkError:
                    emit networkError(job->errorString(), job->rawDataSample(), job->maxRetries(), -1);
                    return;
                default: emit loginError(job->errorString(), job->rawDataSample());
                }
            });
        });
    });
}

void Connection::assumeUnknownIdentity(const QString& accessToken, const QString& refreshToken)
{
    d->ensureHomeserverFromUrl(d->data->baseUrl().toString()).then([this, accessToken, refreshToken] {
        d->data->setToken(accessToken.toLatin1());
        d->data->setRefreshToken(refreshToken);

        this->refreshToken();
        emit refreshTokenChanged();

        callApi<GetTokenOwnerJob>().onResult([this](const GetTokenOwnerJob* job) {
            switch (job->error()) {
            case BaseJob::Success:
                d->data->setDeviceId(job->deviceId());
                d->completeSetup(job->userId());
                return;
            case BaseJob::NetworkError:
                emit networkError(job->errorString(), job->rawDataSample(), job->maxRetries(), -1);
                return;
            default: emit loginError(job->errorString(), job->rawDataSample());
            }
        });
    });
}

JobHandle<GetVersionsJob> Connection::loadVersions()
{
    return callApi<GetVersionsJob>(BackgroundRequest).then([this](GetVersionsJob::Response r) {
        d->data->setSupportedSpecVersions(std::move(r.versions));
    });
}

JobHandle<GetCapabilitiesJob> Connection::loadCapabilities()
{
    return callApi<GetCapabilitiesJob>(BackgroundRequest)
        .then(
            [this](GetCapabilitiesJob::Capabilities response) {
                d->capabilities = std::move(response);
                if (d->capabilities.roomVersions) {
                    qCInfo(MAIN) << "Room versions:" << defaultRoomVersion()
                                 << "is default, full list:" << availableRoomVersions();
                    emit capabilitiesLoaded();
                    for (auto* r : std::as_const(d->roomMap))
                        r->checkVersion();
                } else
                    qCWarning(MAIN) << "The server hasn't reported room versions it supports;"
                                       " version upgrade recommendations won't be issued";
            },
            [](const GetCapabilitiesJob* job) {
                if (job->error() == BaseJob::IncorrectRequest)
                    qCDebug(MAIN) << "The server doesn't support /capabilities;"
                                     " version upgrade recommendations won't be issued";
            });
}

void Connection::reloadCapabilities() { loadCapabilities(); }

bool Connection::loadingCapabilities() const { return !capabilitiesReady(); }

bool Connection::capabilitiesReady() const
{
    // (Ab)use the fact that room versions cannot be omitted after
    // the capabilities have been loaded (see reloadCapabilities() above).
    return d->capabilities.roomVersions.has_value();
}

QStringList Connection::supportedMatrixSpecVersions() const { return d->apiVersions.versions; }

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
        qWarning(MAIN).noquote()
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
        qWarning(MAIN).noquote()
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
        qWarning(MAIN).noquote()
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
        if (encryptionData)
            encryptionData->database.clear();
    });
    connect(loginJob, &BaseJob::failure, q, [this, loginJob] {
        emit q->loginError(loginJob->errorString(), loginJob->rawDataSample());
    });
}

void Connection::Private::completeSetup(const QString& mxId, bool mock)
{
    data->setUserId(mxId);
    q->setObjectName(data->userId() % u'/' % data->deviceId());
    qCDebug(MAIN) << "Using server" << data->baseUrl().toDisplayString()
                  << "by user" << data->userId()
                  << "from device" << data->deviceId();
    connect(qApp, &QCoreApplication::aboutToQuit, q, &Connection::saveState);

    AccountSettings account(mxId);
    account.setKeepLoggedIn(true);
    account.setHomeserver(q->homeserver());
    account.setDeviceId(q->deviceId());
    account.setDeviceName({});
    account.setClientId(data->clientId());
    account.setTokenEndpoint(data->tokenEndpoint());
    account.sync();

    if (!mock) {
        q->loadVersions();
        q->loadCapabilities();
        q->user()->load(); // Load the local user's profile
    }

    if (useEncryption) {
        if (auto&& maybeEncryptionData =
                _impl::ConnectionEncryptionData::setup(q, mock)) {
            encryptionData = std::move(*maybeEncryptionData);
        } else {
            useEncryption = false;
            emit q->encryptionChanged(false);
        }
    } else
        qCInfo(E2EE) << "End-to-end encryption (E2EE) support is off for"
                     << q->objectName();

    emit q->stateChanged();
    emit q->connected();
}

QFuture<void> Connection::Private::ensureHomeserverFromUrl(const QString& url,
                                                    const std::optional<LoginFlow>& flow)
{
    QPromise<void> promise;
    auto result = promise.future();
    promise.start();
    if (data->baseUrl().isValid() && (!flow || loginFlows.contains(*flow))) {
        //q->setObjectName(userId % u"(?)");
        promise.finish(); // Perfect, we're already good to go
    } else {
        // Try to ascertain the homeserver URL and flows
        //q->setObjectName(userId % u"(?)");
        q->resolveServer(url);
        qWarning() << "homeserver" << q->homeserver();
        if (flow)
            QtFuture::connect(q, &Connection::loginFlowsChanged)
                .then([this, flow, p = std::move(promise)]() mutable {
                    if (loginFlows.contains(*flow))
                        p.finish();
                    else // Leave the promise unfinished and emit the error
                        emit q->loginError(tr("Unsupported login flow"),
                                           tr("The homeserver at %1 does not support"
                                              " the login flow '%2'")
                                               .arg(data->baseUrl().toDisplayString(), flow->type));
                });
        else // Any flow is fine, just wait until the homeserver is resolved
            return QFuture<void>(QtFuture::connect(q, &Connection::homeserverChanged));
    }
    return result;
}

QFuture<void> Connection::Private::ensureHomeserver(const QString& userId,
                                                    const std::optional<LoginFlow>& flow)
{
    QPromise<void> promise;
    auto result = promise.future();
    promise.start();
    if (data->baseUrl().isValid() && (!flow || loginFlows.contains(*flow))) {
        q->setObjectName(userId % u"(?)");
        promise.finish(); // Perfect, we're already good to go
    } else if (userId.startsWith(u'@') && userId.indexOf(u':') != -1) {
        // Try to ascertain the homeserver URL and flows
        q->setObjectName(userId % u"(?)");
        q->resolveServer(userId);
        if (flow)
            QtFuture::connect(q, &Connection::loginFlowsChanged)
                .then([this, flow, p = std::move(promise)]() mutable {
                    if (loginFlows.contains(*flow))
                        p.finish();
                    else // Leave the promise unfinished and emit the error
                        emit q->loginError(tr("Unsupported login flow"),
                                           tr("The homeserver at %1 does not support"
                                              " the login flow '%2'")
                                               .arg(data->baseUrl().toDisplayString(), flow->type));
                });
        else // Any flow is fine, just wait until the homeserver is resolved
            return QFuture<void>(QtFuture::connect(q, &Connection::homeserverChanged));
    } else // Leave the promise unfinished and emit the error
        emit q->resolveError(tr("Please provide the fully-qualified user id"
                                " (such as @user:example.org) so that the"
                                " homeserver could be resolved; the current"
                                " homeserver URL(%1) is not good")
                                 .arg(data->baseUrl().toDisplayString()));
    return result;
}

QFuture<void> Connection::logout()
{
    // If there's an ongoing sync job, stop it (this also suspends sync loop)
    const auto wasSyncing = bool(d->syncJob);
    if (wasSyncing)
    {
        d->syncJob->abandon();
        d->syncJob = nullptr;
    }

    d->logoutJob = callApi<LogoutJob>();
    Q_ASSERT(!isLoggedIn()); // Because d->logoutJob is running
    emit stateChanged();

    QFutureInterface<void> p;
    connect(d->logoutJob.get(), &BaseJob::finished, this, [this, wasSyncing, p]() mutable {
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
            Q_ASSERT(isLoggedIn());
            emit stateChanged();
            if (wasSyncing)
                syncLoopIteration(); // Resume sync loop (or a single sync)
            p.cancel();
        }
        p.reportFinished();
    });
    return p.future();
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
        qCInfo(MAIN) << "Timeout for next syncs changed from" << timeout //
                     << "to" << d->syncTimeout;
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
    if (d->encryptionData) {
        d->encryptionData->onSyncSuccess(data);
    }
    d->consumeToDeviceEvents(data.takeToDeviceEvents());
    d->data->setLastEvent(data.nextBatch());
    d->consumeRoomData(data.takeRoomData(), fromCache);
    d->consumeAccountData(data.takeAccountData());
    d->consumePresenceData(data.takePresenceData());
    if(d->encryptionData && d->encryptionData->encryptionUpdateRequired) {
        d->encryptionData->loadOutdatedUserDevices();
        d->encryptionData->encryptionUpdateRequired = false;
    }
    Q_UNUSED(std::move(data)) // Tell static analysers `data` is consumed now
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
                remove_if(directChatMemberIds, [&remoteRemovals, this](auto it) {
                    return remoteRemovals.contains(q->user(it.value()), it.key());
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
                            Q_ASSERT(!directChatMemberIds.contains(it.value(), it.key()));
                            remoteAdditions.insert(u, it.value());
                            directChats.insert(u, it.value());
                            directChatMemberIds.insert(it.value(), it.key());
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
    if (toDeviceEvents.empty())
        return;

    qCDebug(E2EE) << "Consuming" << toDeviceEvents.size() << "to-device events";
    for (auto&& tdEvt : std::move(toDeviceEvents)) {
        if (encryptionData)
            encryptionData->consumeToDeviceEvent(std::move(tdEvt));
    }
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

JobHandle<JoinRoomJob> Connection::joinRoom(const QString& roomAlias, const QStringList& serverNames)
{
    // Upon completion, ensure a room object is created in case it hasn't come with a sync yet.
    // If the room object is not there, provideRoom() will create it in Join state. Using
    // the continuation ensures that the room is provided before any client connections.
    return callApi<JoinRoomJob>(roomAlias, serverNames).then([this](const QString& roomId) {
        provideRoom(roomId);
    });
}

QFuture<Room*> Connection::joinAndGetRoom(const QString& roomAlias, const QStringList& serverNames)
{
    return callApi<JoinRoomJob>(roomAlias, serverNames).then([this](const QString& roomId) {
        return provideRoom(roomId);
    });
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

JobHandle<UploadContentJob> Connection::uploadContent(QIODevice* contentSource,
                                                      const QString& filename,
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

JobHandle<UploadContentJob> Connection::uploadFile(const QString& fileName,
                                         const QString& overrideContentType)
{
    auto sourceFile = new QFile(fileName);
    return uploadContent(sourceFile, QFileInfo(*sourceFile).fileName(),
                         overrideContentType);
}

BaseJob* Connection::getContent(const QString& mediaId)
{
    auto idParts = splitMediaId(mediaId);
    return callApi<DownloadFileJob>(idParts.front(), idParts.back());
}

BaseJob* Connection::getContent(const QUrl& url)
{
    QT_IGNORE_DEPRECATIONS(return getContent(url.authority() + url.path());)
}

DownloadFileJob* Connection::downloadFile(const QUrl& url, const QString& localFilename)
{
    auto mediaId = url.authority() + url.path();
    auto idParts = splitMediaId(mediaId);
    return callApi<DownloadFileJob>(idParts.front(), idParts.back(), localFilename);
}

DownloadFileJob* Connection::downloadFile(
    const QUrl& url, const EncryptedFileMetadata& fileMetadata,
    const QString& localFilename)
{
    auto mediaId = url.authority() + url.path();
    auto idParts = splitMediaId(mediaId);
    return callApi<DownloadFileJob>(idParts.front(), idParts.back(),
                                    fileMetadata, localFilename);
}

JobHandle<CreateRoomJob> Connection::createRoom(
    RoomVisibility visibility, const QString& alias, const QString& name, const QString& topic,
    QStringList invites, const QString& presetName, const QString& roomVersion, bool isDirect,
    const QVector<CreateRoomJob::StateEvent>& initialState,
    const QVector<CreateRoomJob::Invite3pid>& invite3pids, const QJsonObject& creationContent)
{
    invites.removeOne(userId()); // The creator is by definition in the room
    return callApi<CreateRoomJob>(visibility == PublishRoom ? QStringLiteral("public")
                                                            : QStringLiteral("private"),
                                  alias, name, topic, invites, invite3pids, roomVersion,
                                  creationContent, initialState, presetName, isDirect)
        .then(this, [this, invites, isDirect](const QString& roomId) {
            auto* room = provideRoom(roomId, JoinState::Join);
            if (ALARM_X(!room, "Failed to create a room"))
                return;

            emit createdRoom(room);
            if (isDirect)
                for (const auto& i : invites)
                    addToDirectChats(room, i);
        });
}

void Connection::requestDirectChat(const QString& userId)
{
    getDirectChat(userId).then([this](Room* r) { emit directChatAvailable(r); });
}

QFuture<Room*> Connection::getDirectChat(const QString& otherUserId)
{
    auto* u = user(otherUserId);
    if (ALARM_X(!u, u"Couldn't get a user object for" % otherUserId))
        return {};

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
            return QtFuture::makeReadyFuture(r);
        }
        if (auto ir = invitation(roomId)) {
            Q_ASSERT(ir->id() == roomId);
            qCDebug(MAIN) << "Joining the already invited direct chat with" << otherUserId << "at"
                          << roomId;
            return joinAndGetRoom(ir->id());
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
            d->directChatMemberIds.remove(it.value(), it.key()->id());
        }
        emit directChatsListChanged({}, removals);
    }

    return createDirectChat(otherUserId).then([this](const QString& roomId) {
        return room(roomId, JoinState::Join);
    });
}

JobHandle<CreateRoomJob> Connection::createDirectChat(const QString& userId, const QString& topic,
                                                      const QString& name)
{
    QVector<CreateRoomJob::StateEvent> initialStateEvents;

    if (d->encryptDirectChats) {
        const auto encryptionContent = EncryptionEventContent(EncryptionType::MegolmV1AesSha2);
        initialStateEvents.append({ EncryptionEvent::TypeId, encryptionContent.toJson() });
    }

    return createRoom(UnpublishRoom, {}, name, topic, { userId },
                      QStringLiteral("trusted_private_chat"), {}, true, initialStateEvents)
        .then([userId](const QString& roomId) {
            qCDebug(MAIN) << "Direct chat with" << userId << "has been created as" << roomId;
        });
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
                                    generateTxnId(), contents);
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

Avatar& Connection::userAvatar(const QString& avatarMediaId)
{
    return userAvatar(QUrl(avatarMediaId));
}

Avatar& Connection::userAvatar(const QUrl& avatarUrl)
{
    const auto mediaId = avatarUrl.authority() + avatarUrl.path();
    return d->userAvatarMap.try_emplace(mediaId, avatarUrl).first->second;
}

QString Connection::deviceId() const { return d->data->deviceId(); }

QByteArray Connection::accessToken() const
{
    // The logout job needs access token to do its job; so the token is
    // kept inside d->data but no more exposed to the outside world.
    return isJobPending(d->logoutJob) ? QByteArray() : d->data->accessToken();
}

bool Connection::isLoggedIn() const { return !accessToken().isEmpty(); }

QOlmAccount* Connection::olmAccount() const
{
    return d->encryptionData ? &d->encryptionData->olmAccount : nullptr;
}

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
    for (auto* r: std::as_const(d->roomMap))
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
    for (auto* r : std::as_const(d->roomMap)) {
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
    for (auto* r : std::as_const(d->roomMap)) {
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

void Connection::addToDirectChats(const Room* room, const QString& userId)
{
    Q_ASSERT(room != nullptr && !userId.isEmpty());
    const auto u = user(userId);
    if (d->directChats.contains(u, room->id()))
        return;
    Q_ASSERT(!d->directChatMemberIds.contains(room->id(), userId));
    d->directChats.insert(u, room->id());
    d->directChatMemberIds.insert(room->id(), userId);
    d->dcLocalAdditions.insert(u, room->id());
    emit directChatsListChanged({ { u, room->id() } }, {});
}

void Connection::removeFromDirectChats(const QString& roomId, const QString& userId)
{
    Q_ASSERT(!roomId.isEmpty());
    const auto u = user(userId);
    if ((!userId.isEmpty() && !d->directChats.contains(u, roomId))
        || d->directChats.key(roomId) == nullptr)
        return;

    DirectChatsMap removals;
    if (u != nullptr) {
        d->directChats.remove(u, roomId);
        d->directChatMemberIds.remove(roomId, u->id());
        removals.insert(u, roomId);
        d->dcLocalRemovals.insert(u, roomId);
    } else {
        removals = remove_if(d->directChats,
                            [&roomId](auto it) { return it.value() == roomId; });
        d->dcLocalRemovals += removals;
    }
    emit directChatsListChanged({}, removals);
}

bool Connection::isDirectChat(const QString& roomId) const
{
    return d->directChatMemberIds.contains(roomId);
}

QList<QString> Connection::directChatMemberIds(const Room* room) const
{
    Q_ASSERT(room != nullptr);
    return d->directChatMemberIds.values(room->id());
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

void Connection::addToIgnoredUsers(const QString& userId)
{
    auto ignoreList = ignoredUsers();
    if (!ignoreList.contains(userId)) {
        ignoreList.insert(userId);
        d->packAndSendAccountData<IgnoredUsersEvent>(ignoreList);
        emit ignoredUsersListChanged({ { userId } }, {});
    }
}

void Connection::removeFromIgnoredUsers(const QString& userId)
{
    auto ignoreList = ignoredUsers();
    if (ignoreList.remove(userId) != 0) {
        d->packAndSendAccountData<IgnoredUsersEvent>(ignoreList);
        emit ignoredUsersListChanged({}, { { userId } });
    }
}

QStringList Connection::userIds() const { return d->userMap.keys(); }

const ConnectionData* Connection::connectionData() const
{
    return d->data.get();
}

HomeserverData Connection::homeserverData() const { return d->data->homeserverData(); }

Room* Connection::provideRoom(const QString& id, std::optional<JoinState> joinState)
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
            for (const auto dcMembers = prevInvite->directChatMembers(); const auto& m : dcMembers)
                addToDirectChats(room, m.id());
            qCDebug(MAIN) << "Deleting Invite state for room"
                          << prevInvite->id();
            emit prevInvite->beforeDestruction(prevInvite);
            prevInvite->deleteLater();
        }
    }

    return room;
}

void Connection::setEncryptionDefault(bool useByDefault)
{
    Private::encryptionDefault = useByDefault;
}

void Connection::setDirectChatEncryptionDefault(bool useByDefault)
{
    Private::directChatEncryptionDefault = useByDefault;
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

QString Connection::generateTxnId() const
{
    return d->data->generateTxnId();
}

QFuture<QList<LoginFlow>> Connection::setHomeserver(const QUrl& baseUrl)
{
    d->resolverJob.abandon();
    d->loginFlowsJob.abandon();
    d->loginFlows.clear();

    if (homeserver() != baseUrl) {
        d->data->setBaseUrl(baseUrl);
        emit homeserverChanged(homeserver());
    }

    d->loginFlowsJob = callApi<GetLoginFlowsJob>(BackgroundRequest).onResult([this] {
        if (d->loginFlowsJob->status().good())
            d->loginFlows = d->loginFlowsJob->flows();
        else
            d->loginFlows.clear();
        emit loginFlowsChanged();
    });
    return d->loginFlowsJob.responseFuture();
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
        for (const auto* r: std::as_const(d->roomMap)) {
            if (r->joinState() == JoinState::Leave)
                continue;
            (r->joinState() == JoinState::Invite ? inviteRoomsJson : roomsJson)
                .insert(r->id(),
                        QJsonObject{ { "$ref"_ls,
                                       SyncData::fileNameForRoom(r->id()) } });
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

    if (d->encryptionData) {
        QJsonObject keysJson = toJson(d->encryptionData->oneTimeKeysCount);
        rootObj.insert(QStringLiteral("device_one_time_keys_count"), keysJson);
    }

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

bool Connection::encryptionEnabled() const
{
    return d->useEncryption;
}

void Connection::enableEncryption(bool enable)
{
    if (enable == d->useEncryption)
        return;

    if (isLoggedIn()) {
        qWarning(E2EE) << "It's only possible to enable/disable E2EE "
                          "before logging in; the account"
                       << objectName()
                       << "is already logged in, the E2EE state will remain"
                       << d->useEncryption;
        return;
    }

    d->useEncryption = enable;
    emit encryptionChanged(enable);
}

bool Connection::directChatEncryptionEnabled() const
{
    return d->encryptDirectChats;
}

void Connection::enableDirectChatEncryption(bool enable)
{
    if (enable == d->encryptDirectChats) {
        return;
    }

    d->encryptDirectChats = enable;
    emit directChatsEncryptionChanged(enable);
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

bool Connection::isQueryingKeys() const
{
    return d->encryptionData
           && d->encryptionData->currentQueryKeysJob != nullptr;
}

void Connection::encryptionUpdate(const Room* room, const QStringList& invitedIds)
{
    if (d->encryptionData) {
        d->encryptionData->encryptionUpdate(room->joinedMemberIds() + invitedIds);
    }
}

QFuture<QByteArray> Connection::requestKeyFromDevices(event_type_t name)
{
    QPromise<QByteArray> keyPromise;
    keyPromise.setProgressRange(0, 10);
    keyPromise.start();

    UsersToDevicesToContent content;
    const auto& requestId = generateTxnId();
    const QJsonObject eventContent{ { "action"_ls, "request"_ls },
                                    { "name"_ls, name },
                                    { "request_id"_ls, requestId },
                                    { "requesting_device_id"_ls, deviceId() } };
    for (const auto& deviceId : devicesForUser(userId())) {
        content[userId()][deviceId] = eventContent;
    }
    sendToDevices("m.secret.request"_ls, content);
    auto futureKey = keyPromise.future();
    keyPromise.setProgressValue(5); // Already sent the request, now it's only to get the result
    connectUntil(this, &Connection::secretReceived, this,
                 [this, requestId, name, kp = std::move(keyPromise)](
                     const QString& receivedRequestId, const QString& secret) mutable {
                     if (requestId != receivedRequestId) {
                         return false;
                     }
                     const auto& key = QByteArray::fromBase64(secret.toLatin1());
                     database()->storeEncrypted(name, key);
                     kp.addResult(key);
                     kp.finish();
                     return true;
                 });
    return futureKey;
}

QJsonObject Connection::decryptNotification(const QJsonObject& notification)
{
    if (auto r = room(notification[RoomIdKey].toString()))
        if (auto event =
                loadEvent<EncryptedEvent>(notification["event"_ls].toObject()))
            if (const auto decrypted = r->decryptMessage(*event))
                return decrypted->fullJson();
    return {};
}

Database* Connection::database() const
{
    return d->encryptionData ? &d->encryptionData->database : nullptr;
}

std::unordered_map<QByteArray, QOlmInboundGroupSession> Connection::loadRoomMegolmSessions(const Room* room) const
{
    return database()->loadMegolmSessions(room->id());
}

void Connection::saveMegolmSession(const Room* room,
                                   const QOlmInboundGroupSession& session, const QByteArray& senderKey, const QByteArray& senderEdKey) const
{
    database()->saveMegolmSession(room->id(), session, senderKey, senderEdKey);
}

QStringList Connection::devicesForUser(const QString& userId) const
{
    return d->encryptionData->deviceKeys.value(userId).keys();
}

QString Connection::edKeyForUserDevice(const QString& userId,
                                       const QString& deviceId) const
{
    return d->encryptionData->deviceKeys[userId][deviceId]
        .keys["ed25519:"_ls + deviceId];
}

QString Connection::curveKeyForUserDevice(
    const QString& userId, const QString& device) const
{
    return d->encryptionData->curveKeyForUserDevice(userId, device);
}

bool Connection::hasOlmSession(const QString& user,
                               const QString& deviceId) const
{
    return d->encryptionData && d->encryptionData->hasOlmSession(user, deviceId);
}

void Connection::sendSessionKeyToDevices(
    const QString& roomId, const QOlmOutboundGroupSession& outboundSession,
    const QMultiHash<QString, QString>& devices)
{
    Q_ASSERT(d->encryptionData != nullptr);
    d->encryptionData->sendSessionKeyToDevices(roomId, outboundSession, devices);
}

KeyVerificationSession* Connection::startKeyVerificationSession(const QString& userId,
                                                                const QString& deviceId)
{
    if (!d->encryptionData) {
        qWarning(E2EE) << "E2EE is switched off on" << objectName()
                       << "- you can't start a verification session on it";
        return nullptr;
    }
    return d->encryptionData->setupKeyVerificationSession(userId, deviceId,
                                                          this);
}

void Connection::sendToDevice(const QString& targetUserId,
                              const QString& targetDeviceId, const Event& event,
                              bool encrypted)
{
    if (encrypted && !d->encryptionData) {
        qWarning(E2EE) << "E2EE is off for" << objectName()
                       << "- no encrypted to-device message will be sent";
        return;
    }

    const auto contentJson =
        encrypted
            ? d->encryptionData->assembleEncryptedContent(event.fullJson(),
                                                          targetUserId,
                                                          targetDeviceId)
            : event.contentJson();
    sendToDevices(encrypted ? EncryptedEvent::TypeId : event.matrixType(),
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
    const auto olmSessionId = query.value("olmSessionId"_ls).toString();
    if (olmSessionId == "BACKUP_VERIFIED"_ls) {
        return true;
    }
    if (olmSessionId == "SELF"_ls) {
        return true;
    }
    query.prepare("SELECT senderKey FROM olm_sessions WHERE sessionId=:sessionId;"_ls);
    query.bindValue(":sessionId"_ls, olmSessionId.toLatin1());
    database()->execute(query);
    if (!query.next()) {
        return false;
    }
    const auto curveKey = query.value("senderKey"_ls).toString();

    query.prepare("SELECT matrixId, selfVerified, verified FROM tracked_devices WHERE curveKey=:curveKey;"_ls);
    query.bindValue(":curveKey"_ls, curveKey);
    database()->execute(query);
    if (!query.next()) {
        return false;
    }
    const auto userId = query.value("matrixId"_ls).toString();
    return query.value("verified"_ls).toBool() || (isUserVerified(userId) && query.value("selfVerified"_ls).toBool());
}

QString Connection::masterKeyForUser(const QString& userId) const
{
    auto query = database()->prepareQuery("SELECT key FROM master_keys WHERE userId=:userId"_ls);
    query.bindValue(":userId"_ls, userId);
    database()->execute(query);
    return query.next() ? query.value("key"_ls).toString() : QString();
}

bool Connection::isUserVerified(const QString& userId) const
{
    auto query = database()->prepareQuery("SELECT verified FROM master_keys WHERE userId=:userId"_ls);
    query.bindValue(":userId"_ls, userId);
    database()->execute(query);
    return query.next() && query.value("verified"_ls).toBool();
}

bool Connection::isVerifiedDevice(const QString& userId, const QString& deviceId) const
{
    auto query = database()->prepareQuery("SELECT verified, selfVerified FROM tracked_devices WHERE deviceId=:deviceId AND matrixId=:matrixId;"_ls);
    query.bindValue(":deviceId"_ls, deviceId);
    query.bindValue(":matrixId"_ls, userId);
    database()->execute(query);
    if (!query.next()) {
        return false;
    }
    return query.value("verified"_ls).toBool() || (isUserVerified(userId) && query.value("selfVerified"_ls).toBool());
}

bool Connection::isKnownE2eeCapableDevice(const QString& userId, const QString& deviceId) const
{
    auto query = database()->prepareQuery("SELECT verified FROM tracked_devices WHERE deviceId=:deviceId AND matrixId=:matrixId;"_ls);
    query.bindValue(":deviceId"_ls, deviceId);
    query.bindValue(":matrixId"_ls, userId);
    database()->execute(query);
    return query.next();
}

bool Connection::hasConflictingDeviceIdsAndCrossSigningKeys(const QString& userId)
{
    if (d->encryptionData) {
        return d->encryptionData->hasConflictingDeviceIdsAndCrossSigningKeys(userId);
    }
    return true;
}

void Connection::reloadDevices()
{
    if (d->encryptionData) {
        d->encryptionData->reloadDevices();
    }
}

Connection* Connection::makeMockConnection(const QString& mxId,
                                           bool enableEncryption)
{
    auto* c = new Connection;
    c->enableEncryption(enableEncryption);
    c->d->completeSetup(mxId, true);
    return c;
}

QStringList Connection::accountDataEventTypes() const
{
    QStringList events;
    events.reserve(d->accountData.size());
    for (const auto& [key, value] : std::as_const(d->accountData)) {
        events += key;
    }
    return events;
}

void Connection::startSelfVerification()
{
    auto query = database()->prepareQuery("SELECT deviceId FROM tracked_devices WHERE matrixId=:matrixId AND selfVerified=1;"_ls);
    query.bindValue(":matrixId"_ls, userId());
    database()->execute(query);
    QStringList devices;
    while(query.next()) {
        auto id = query.value("deviceId"_ls).toString();
        if (id != deviceId()) {
            devices += id;
        }
    }
    for (const auto &device : devices) {
        auto session = new KeyVerificationSession(userId(), device, this);
        d->encryptionData->verificationSessions[session->transactionId()] = session;
        connect(session, &QObject::destroyed, this, [this, session] {
            d->encryptionData->verificationSessions.remove(session->transactionId());
        });
        connectUntil(this, &Connection::keyVerificationStateChanged, this, [session, this](const auto &changedSession, const auto state){
            if (changedSession->transactionId() == session->transactionId() && state != KeyVerificationSession::CANCELED) {
                emit newKeyVerificationSession(session);
                return true;
            }
            return state == KeyVerificationSession::CANCELED;
        });
    }
}

bool Connection::allSessionsSelfVerified(const QString& userId) const
{
    auto query = database()->prepareQuery("SELECT deviceId FROM tracked_devices WHERE matrixId=:matrixId AND selfVerified=0;"_ls);
    query.bindValue(":matrixId"_ls, userId);
    database()->execute(query);
    return !query.next();
}

void Connection::startOidcLogin()
{
    auto nam = new QNetworkAccessManager(this); //TODO replace with global nam
    auto url = QUrl::fromUserInput(d->data->baseUrl().toString());
    url.setPath(u"/.well-known/matrix/client"_s);

    qWarning() << url << d->data->baseUrl();

    auto reply = nam->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [reply, nam, this] {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << 1 << reply->readAll() << reply->error() << reply->errorString();
            return;
        }
        auto wellKnownJson = QJsonDocument::fromJson(reply->readAll()).object(); //TODO replace with wellknown job
        auto configurationUrl = QUrl(u"%1/.well-known/openid-configuration"_s.arg(wellKnownJson[u"org.matrix.msc2965.authentication"_s][u"issuer"_s].toString()));

        auto configurationReply = nam->get(QNetworkRequest(configurationUrl));
        connect(configurationReply, &QNetworkReply::finished, this, [configurationReply, this, nam] {
            if (configurationReply->error() != QNetworkReply::NoError) {
                qWarning() << 2 << configurationReply->readAll() << configurationReply->error() << configurationReply->errorString();
                return;
            }
            auto configuration = QJsonDocument::fromJson(configurationReply->readAll()).object();
            auto registrationEndpoint = configuration[u"registration_endpoint"_s].toString();
            auto authorizationEndpoint = configuration[u"authorization_endpoint"_s].toString();
            auto tokenEndpoint = configuration[u"token_endpoint"_s].toString();
            d->data->setTokenEndpoint(tokenEndpoint);

            QJsonObject clientData = {
                {u"client_name"_s, u"NeoChat"_s},
                {u"client_uri"_s, u"https://apps.kde.org/neochat"_s},
                {u"logo_uri"_s, u"https://apps.kde.org/app-icons/org.kde.neochat.svg"_s},
                {u"contacts"_s, QJsonArray {u"#neochat:kde.org"_s}},
                {u"tos_uri"_s, u"https://apps.kde.org/neochat"_s},
                {u"policy_uri"_s, u"https://apps.kde.org/neochat"_s},
                {u"redirect_uris"_s, QJsonArray {u"http://localhost:8634"_s}},
                {u"application_type"_s, u"native"_s},
                {u"grant_types"_s, QJsonArray {u"authorization_code"_s, u"refresh_token"_s}},
                {u"response_types"_s, QJsonArray {u"code"_s}},
                {u"token_endpoint_auth_method"_s, u"none"_s}
            };
            auto registrationUrl = QUrl(registrationEndpoint);

            QNetworkRequest request(registrationUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_s);

            auto registrationReply = nam->post(request, QJsonDocument(clientData).toJson(QJsonDocument::Compact));

            connect(registrationReply, &QNetworkReply::finished, this, [registrationReply, authorizationEndpoint, tokenEndpoint, nam, this] {
                if (registrationReply->error() != QNetworkReply::NoError) {
                    qWarning() << 3 << registrationReply->readAll() << registrationReply->error() << registrationReply->errorString();
                    return;
                }
                auto registrationResponseJson = QJsonDocument::fromJson(registrationReply->readAll()).object();
                auto clientId = registrationResponseJson[u"client_id"_s].toString();
                d->data->setClientId(clientId);

                QUrl authUrl(authorizationEndpoint);
                QUrlQuery query;

                auto state = QString::number(QRandomGenerator::securelySeeded().generate64());
                auto verifier = QCryptographicHash::hash(QStringLiteral("Random %1").arg(QRandomGenerator::securelySeeded().generate64()).toLatin1(), QCryptographicHash::Sha256).toHex();
                auto challenge = QString::fromLatin1(QCryptographicHash::hash(verifier, QCryptographicHash::Sha256).toBase64());
                auto challengeString = challenge.replace(QLatin1Char('+'), QLatin1Char('-')).replace(QLatin1Char('/'), QLatin1Char('_')).left(challenge.indexOf(QLatin1Char('=')));
                auto nonce = QString::fromLatin1(QString::number(QRandomGenerator::securelySeeded().generate64()).toLatin1().toHex());

                query.addQueryItem(u"response_mode"_s, u"query"_s);
                query.addQueryItem(u"response_type"_s, u"code"_s);
                query.addQueryItem(u"redirect_uri"_s, u"http://localhost:8634"_s);
                query.addQueryItem(u"client_id"_s, clientId);
                query.addQueryItem(u"state"_s, state);
                query.addQueryItem(u"scope"_s, u"openid+urn:matrix:org.matrix.msc2967.client:api:*+urn:matrix:org.matrix.msc2967.client:device:ABCDEFGHIJKP"_s);
                query.addQueryItem(u"nonce"_s, nonce);
                query.addQueryItem(u"code_challenge_method"_s, u"S256"_s);
                query.addQueryItem(u"code_challenge"_s, challengeString);

                authUrl.setQuery(query);

                emit openOidcUrl(authUrl.toString());

                auto server = new QTcpServer(this);
                server->listen(QHostAddress(QStringLiteral("127.0.0.1")), 8634);
                connect(server, &QTcpServer::newConnection, this, [this, server, state, verifier, tokenEndpoint, nam, clientId](){
                    auto connection = server->nextPendingConnection();
                    connection->waitForReadyRead();
                    auto data = QString::fromLatin1(connection->readAll());
                    connection->write("HTTP/1.0 200 OK\r\n\r\nYou can return to NeoChat now.");
                    connection->close();
                    QRegularExpression codeRegex(QStringLiteral("code=([a-zA-Z0-9]+) "));
                    auto code = codeRegex.match(data).captured(1);
                    QRegularExpression stateRegex(QStringLiteral("state=([0-9]+)"));
                    if (stateRegex.match(data).captured(1) != state) {
                        return;
                    }
                    QUrlQuery query;
                    query.addQueryItem(QLatin1String("grant_type"), QStringLiteral("authorization_code"));
                    query.addQueryItem(QLatin1String("code"), QString::fromLatin1(QUrl::toPercentEncoding(code)));
                    query.addQueryItem(QLatin1String("redirect_uri"), QString::fromLatin1(QUrl::toPercentEncoding(QStringLiteral("http://localhost:8634"))));
                    query.addQueryItem(QLatin1String("code_verifier"), QString::fromLatin1(verifier));
                    query.addQueryItem(QLatin1String("client_id"), clientId);

                    QNetworkRequest request((QUrl(tokenEndpoint)));
                    request.setHeader(QNetworkRequest::UserAgentHeader, u"NeoChat"_s);
                    request.setHeader(QNetworkRequest::ContentTypeHeader, u"application/x-www-form-urlencoded"_s);
                    auto reply = nam->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
                    connect(reply, &QNetworkReply::finished, this, [reply, clientId, tokenEndpoint, this] {
                        auto json = QJsonDocument::fromJson(reply->readAll()).object();
                        auto accessToken = json[u"access_token"_s].toString();
                        auto refreshToken = json[u"refresh_token"_s].toString();

                        assumeUnknownIdentity(accessToken, refreshToken);
                    });
                });
            });
        });
    });
}

void Connection::refreshToken()
{
    QUrlQuery query;
    query.addQueryItem(u"grant_type"_s, u"refresh_token"_s);
    query.addQueryItem(u"refresh_token"_s, QString::fromLatin1(QUrl::toPercentEncoding(d->data->refreshToken())));
    query.addQueryItem(u"client_id"_s, d->data->clientId());

    qWarning() << d->data->refreshToken();

    QNetworkRequest request((QUrl(d->data->tokenEndpoint())));
    request.setHeader(QNetworkRequest::UserAgentHeader, u"NeoChat"_s);
    request.setHeader(QNetworkRequest::ContentTypeHeader, u"application/x-www-form-urlencoded"_s);
    auto reply = d->data->nam()->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [reply, this](){
        auto json = QJsonDocument::fromJson(reply->readAll()).object();
        qWarning() << json;
        auto accessToken = json[u"access_token"_s].toString();
        auto refreshToken = json[u"refresh_token"_s].toString();
        auto expires = json[u"expires_in"].toInt();
        d->data->setToken(accessToken.toLatin1());
        qWarning() << "???" << refreshToken;
        d->data->setRefreshToken(refreshToken);
        QTimer::singleShot(expires * 900, this, &Connection::refreshToken);
        emit refreshTokenChanged();
    });
}

// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2017 Roman Plášil <me@rplasil.name>
// SPDX-FileCopyrightText: 2019 Ville Ranki <ville.ranki@iki.fi>
// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "connection.h"

#include "connection_p.h"
#include "connectiondata.h"
#include "logging_categories_p.h"
#include "qt_connection_util.h"
#include "ranges_extras.h"
#include "room.h"
#include "settings.h"
#include "user.h"
#include "rust_util.h"
#include "events/roommemberevent.h"
#include "events/keyverificationevent.h"
#include "e2ee/cryptoutils.h"

#include "csapi/account-data.h"
#include "csapi/cross_signing.h"
#include "csapi/joining.h"
#include "csapi/leaving.h"
#include "csapi/logout.h"
#include "csapi/room_send.h"
#include "csapi/to_device.h"
#include "csapi/voip.h"
#include "csapi/wellknown.h"
#include "csapi/whoami.h"
#include "csapi/keys.h"
#include "csapi/key_backup.h"

#include "events/directchatevent.h"
#include "events/encryptionevent.h"
#include "jobs/downloadfilejob.h"
#include "jobs/mediathumbnailjob.h"

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
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtNetwork/QDnsLookup>
#include <qt6keychain/keychain.h>

#include <ranges>

using namespace Quotient;

namespace {
// This is very much Qt-specific; STL iterators don't have key() and value()
template <typename HashT>
HashT remove_if(HashT& hashMap,
                std::invocable<typename HashT::key_type, typename HashT::value_type> auto pred)
{
    HashT removals;
    for (auto it = hashMap.begin(); it != hashMap.end();) {
        if (pred(it.key(), it.value())) {
            removals.insert(it.key(), it.value());
            it = hashMap.erase(it);
        } else
            ++it;
    }
    return removals;
}

inline void map_subtract(auto& lhs, const auto& rhs)
{
    remove_if(lhs, [&rhs](const auto& k, const auto& v) { return rhs.contains(k, v); });
}
}

Connection::Connection(const QUrl& server, QObject* parent)
    : QObject(parent)
    , d(makeImpl<Private>(std::make_unique<ConnectionData>(server)))
{
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

    auto maybeBaseUrl = QUrl::fromUserInput(serverPart(mxid));
    maybeBaseUrl.setScheme("https"_L1); // Instead of the Qt-default "http"
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
    return { u"m.id.user"_s, { { u"user"_s, id } } };
}

inline UserIdentifier make3rdPartyIdentifier(const QString& medium,
                                             const QString& address)
{
    return { u"m.id.thirdparty"_s, { { u"medium"_s, medium }, { u"address"_s, address } } };
}

void Connection::loginWithPassword(const QString& userId,
                                   const QString& password,
                                   const QString& initialDeviceName,
                                   const QString& deviceId)
{
    d->ensureHomeserver(userId, LoginFlowTypes::Password).then([=, this] {
        setObjectName(userId % u"(?)");
        d->loginToServer(LoginFlowTypes::Password, makeUserIdentifier(userId),
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
    Q_ASSERT(d->data->baseUrl().isValid() && d->supportsLoginFlow(LoginFlowTypes::Token));
    setObjectName(loginToken % u"(?)");
    d->loginToServer(LoginFlowTypes::Token, std::nullopt /*user is encoded in loginToken*/,
                     QString() /*password*/, loginToken, deviceId, initialDeviceName);
}

void Connection::assumeIdentity(const QString& mxId, const QString& deviceId,
                                const QString& accessToken)
{
    d->completeSetup(mxId, false, deviceId, accessToken);

    d->ensureHomeserver(mxId).then([this, mxId] {
        callApi<GetTokenOwnerJob>().onResult([this, mxId](const GetTokenOwnerJob* job) {
            switch (job->error()) {
            case BaseJob::Success:
                if (mxId != job->userId())
                    qCWarning(MAIN).nospace()
                        << "The access_token owner (" << job->userId()
                        << ") is different from passed MXID (" << mxId << ")!";
                return;
            case BaseJob::NetworkError:
                emit networkError(
                    job->errorString(), job->rawDataSample(),
                    static_cast<int>(job->currentBackoffStrategy().maxRetries.value_or(-1)), -1);
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

bool Connection::capabilitiesReady() const
{
    // (Ab)use the fact that room versions cannot be omitted after
    // the capabilities have been loaded (see reloadCapabilities() above).
    return d->capabilities.roomVersions.has_value();
}

QStringList Connection::supportedMatrixSpecVersions() const { return d->data->homeserverData().supportedSpecVersions; }

namespace {
QFuture<QKeychain::Job*> runKeychainJob(QKeychain::Job* j, const QString& keychainId)
{
    j->setAutoDelete(true);
    j->setKey(keychainId);
    auto ft = QtFuture::connect(j, &QKeychain::Job::finished);
    j->start();
    return ft;
}
}

void Connection::Private::saveAccessTokenToKeychain() const
{
    qCDebug(MAIN) << "Saving access token to keychain for" << q->userId();
    using namespace QKeychain;
    auto job = new WritePasswordJob(qAppName());
    job->setBinaryData(data->accessToken());
    runKeychainJob(job, q->userId()).then([](const Job* j) {
        if (j->error() == Error::NoError)
            return;
        qWarning(MAIN).noquote() << "Could not save access token to the keychain:"
                                 << qUtf8Printable(j->errorString());
        // TODO: emit a signal
    });
}

void Connection::Private::dropAccessToken()
{
    // TODO: emit a signal on important (i.e. access denied) keychain errors
    using namespace QKeychain;
    qCDebug(MAIN) << "Removing access token and pickle from keychain for" << q->userId();
    runKeychainJob(new DeletePasswordJob(qAppName()), q->userId()).then([](const Job* job) {
        if (job->error() == Error::NoError || job->error() == Error::EntryNotFound)
            return;
        qWarning(MAIN).noquote() << "Could not delete access token from the keychain:"
                                 << qUtf8Printable(job->errorString());
    });

    runKeychainJob(new DeletePasswordJob(qAppName()), q->userId() + "-Pickle"_L1)
        .then([](const Job* job) {
        if (job->error() == Error::NoError
            || job->error() == Error::EntryNotFound)
            return;
        qWarning(MAIN).noquote()
            << "Could not delete account pickle from the keychain:"
            << qUtf8Printable(job->errorString());
    });

    data->setAccessToken({});
}

template <typename... LoginArgTs>
void Connection::Private::loginToServer(LoginArgTs&&... loginArgs)
{
    q->callApi<LoginJob>(std::forward<LoginArgTs>(loginArgs)...)
        .onResult([this](const LoginJob* loginJob) {
            if (loginJob->status().good()) {
                completeSetup(loginJob->userId(), true, loginJob->deviceId(),
                              loginJob->accessToken());
            } else
                emit q->loginError(loginJob->errorString(), loginJob->rawDataSample());
        });
}

QFuture<void> Connection::Private::setupPicklingKey()
{
    using namespace QKeychain;
    const auto keychainId = q->userId() + "-Pickle"_L1;
    qCInfo(MAIN) << "Keychain request: app" << qAppName() << "id" << keychainId;

    QPointer<Connection> that(q);

    return runKeychainJob(new ReadPasswordJob(qAppName()), keychainId)
        .then([keychainId, this, that](const Job* j) -> QFuture<Job*> {
            // The future will hold nullptr if the existing pickling key was found and no write is
            // pending; a pointer to the write job if if a new key was made and is being written;
            // be cancelled in case of an error.
            switch (const auto readJob = static_cast<const ReadPasswordJob*>(j); readJob->error()) {
            case Error::NoError: {
                auto&& data = readJob->binaryData();
                qDebug(E2EE) << "Successfully loaded pickling key from keychain";

                if (that) {
                    setupCryptoMachine(data);
                }
                return QtFuture::makeReadyValueFuture<Job*>(nullptr);
            }
            case Error::EntryNotFound: {
                auto&& picklingKey = PicklingKey::generate();
                auto writeJob = new WritePasswordJob(qAppName());
                const auto base64key = picklingKey.toBase64();
                if (that) {
                    setupCryptoMachine(base64key);
                }
                writeJob->setBinaryData(base64key);
                qDebug(E2EE) << "Saving a new pickling key to the keychain";
                return runKeychainJob(writeJob, keychainId);
            }
            default:
                qWarning(E2EE) << "Error loading pickling key - please fix your keychain:"
                               << readJob->errorString();
            }
            return {};
        })
        .unwrap()
        .then([](QFuture<Job*> writeFuture) {
            if (const Job* const writeJob = writeFuture.result();
                writeJob && writeJob->error() != Error::NoError)
            {
                qCritical(E2EE) << "Could not save pickling key to keychain: "
                                << writeJob->errorString();
                writeFuture.cancel();
            }
        });
}

void Connection::Private::setupCryptoMachine(const QByteArray& picklingKey)
{
    auto mxIdForDb = q->userId();
    mxIdForDb.replace(u':', u'_');
    const QString databaseFolder{ QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) % u'/' % mxIdForDb };
    const QString legacyDatabaseFile{ databaseFolder + "/quotient_%1.db3"_L1.arg(q->deviceId()) };
    const auto hasVodozemacDatabase = QDir().exists(databaseFolder + u'/' + q->deviceId());
    if (hasVodozemacDatabase && QFile::exists(legacyDatabaseFile)) {
        qCDebug(E2EE) << "Removing legacy database as new database already exists";
        QFile(legacyDatabaseFile).remove();
    }

    QString accountPickle;
    const auto hasDb = QFileInfo(legacyDatabaseFile).exists();
    if (hasDb) {
        auto db = QSqlDatabase::addDatabase(u"QSQLITE"_s, "Quotient_"_L1 + q->deviceId());
        QDir(databaseFolder).mkpath("."_L1);
        db.setDatabaseName(legacyDatabaseFile);
        db.open();
        QSqlQuery query(db);
        query.prepare(u"SELECT pickle FROM accounts;"_s);
        query.exec();
        query.next();
        accountPickle = query.value(0).toString();
        db.close();
    }

    cryptoMachine = crypto::init(stringToRust(q->userId()), stringToRust(q->deviceId()),
                                 stringToRust(databaseFolder % u'/' % q->deviceId()),
                                 bytesToRust(picklingKey.toBase64()), stringToRust(accountPickle));
    if (!(*cryptoMachine)->is_ok()) {
        qCritical() << "Failed to load crypto machine"
                    << static_cast<int>((*cryptoMachine)->error())
                    << stringFromRust((*cryptoMachine)->error_string());
        qApp->exit(1);
        return;
    }

    if (hasDb) {
        auto db = QSqlDatabase::addDatabase(u"QSQLITE"_s, "Quotient_"_L1 + q->deviceId());
        QDir(databaseFolder).mkpath("."_L1);
        db.setDatabaseName(legacyDatabaseFile);
        db.open();

        QSqlQuery query(db);
        query.prepare(u"SELECT * FROM inbound_megolm_sessions;"_s);
        query.exec();
        rust::Vec<MegolmSessionData> megolmSessions;
        while(query.next()) {
            megolmSessions.push_back(MegolmSessionData {
               .libolm_pickle = stringToRust(query.value(u"pickle"_s).toString()),
               .sender_curve_key = stringToRust(query.value(u"senderKey"_s).toString()),
               .room_id = stringToRust(query.value(u"roomId"_s).toString()),
            });
        }
        (*cryptoMachine)->inbound_from_libolm_pickle(megolmSessions, bytesToRust(picklingKey.toBase64()));

        query.prepare(u"SELECT * FROM olm_sessions;"_s);
        query.exec();
        rust::Vec<OlmSessionData> sessions;
        while(query.next()) {
            sessions.push_back(OlmSessionData {
                .libolm_pickle = stringToRust(query.value(u"pickle"_s).toString()),
                .sender_key = stringToRust(query.value(u"senderKey"_s).toString()),
                .creation_time = std::uint64_t(query.value(u"lastReceived"_s).toDateTime().toSecsSinceEpoch()), // This is a lie
                .last_use_time = std::uint64_t(query.value(u"lastReceived"_s).toDateTime().toSecsSinceEpoch()),
            });
        }
        (*cryptoMachine)->import_olm_sessions(sessions, bytesToRust(picklingKey.toBase64()));

        connect(q, &Connection::finishedQueryingKeys, q, [this, db, picklingKey, legacyDatabaseFile]{
            q->callApi<GetRoomKeysVersionCurrentJob>().onResult([db, picklingKey, this](const auto& job) {
                auto loadEncrypted = [db, picklingKey](const auto &name){
                    QSqlQuery query(db);
                    query.prepare(u"SELECT cipher, iv FROM encrypted WHERE name=:name;"_s);
                    query.bindValue(u":name"_s, name);
                    query.exec();
                    if (!query.next()) {
                        return QByteArray();
                    }
                    auto cipher = QByteArray::fromBase64(query.value(u"cipher"_s).toString().toLatin1());
                    auto iv = QByteArray::fromBase64(query.value(u"iv"_s).toString().toLatin1());
                    if (iv.size() < 16) {
                        qCWarning(E2EE) << "Corrupt iv at the database record for" << name;
                        return QByteArray();
                    }

                    return aesCtr256Decrypt(cipher, asCBytes(picklingKey).first<Aes256KeySize>(), asCBytes<AesBlockSize>(iv)).value_or(QByteArray());
                };
                auto selfSigningKey = loadEncrypted(u"m.cross_signing.self_signing"_s).toBase64();
                auto userSigningKey = loadEncrypted(u"m.cross_signing.user_signing"_s).toBase64();
                auto masterKey = loadEncrypted(u"m.cross_signing.master"_s).toBase64();
                auto backupKey = loadEncrypted(u"m.megolm_backup.v1"_s).toBase64();
                (*cryptoMachine)->migrate_secrets(bytesToRust(masterKey), bytesToRust(selfSigningKey), bytesToRust(userSigningKey), bytesToRust(backupKey), stringToRust(job->version()));
            });
            QFile(legacyDatabaseFile).remove();
            processOutgoingRequests();
        });
    } else {
        processOutgoingRequests();
    }
}

void Connection::Private::completeSetup(const QString& mxId, bool newLogin,
                                        const std::optional<QString>& deviceId,
                                        const std::optional<QString>& accessToken)
{
    q->setObjectName(data->userId() % u'/' % data->deviceId());
    data->setIdentity(mxId, deviceId.value_or(u""_s), accessToken.value_or(u""_s).toLatin1());
    qCDebug(MAIN) << "Using server" << data->baseUrl().toDisplayString()
                << "by user" << data->userId()
                << "from device" << data->deviceId();
    connect(qApp, &QCoreApplication::aboutToQuit, q, &Connection::saveState);

    if (newLogin) {
        saveAccessTokenToKeychain();
    }

    if (accessToken.has_value()) {
        q->loadVersions();
        q->loadCapabilities();
        q->user()->load(); // Load the local user's profile
    }
    auto doCompleteSetup = [this, mxId, deviceId, accessToken](QKeychain::Job* = nullptr){
        setupPicklingKey();

        emit q->stateChanged();

        if (useEncryption) {
            emit q->encryptionChanged(useEncryption);
            emit q->stateChanged();
            emit q->ready();
            emit q->connected();
        } else {
            qCInfo(E2EE) << "End-to-end encryption (E2EE) support is off for" << q->objectName();
            emit q->ready();
            emit q->connected();
        }
    };
    if (newLogin) {
        auto mxIdForDb = q->userId();
        mxIdForDb.replace(u':', u'_');
        const QString databasePath{ QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) % u'/' % mxIdForDb % u'/' % q->deviceId() };
        QDir(databasePath).removeRecursively();
        runKeychainJob(new QKeychain::ReadPasswordJob(qAppName()), q->userId() % u"-Pickle"_s).then([doCompleteSetup, this](auto *job) {
            if (job->error() != QKeychain::EntryNotFound) {
                runKeychainJob(new QKeychain::DeletePasswordJob(qAppName()), q->userId() % u"-Pickle"_s).then(doCompleteSetup);
            } else {
                doCompleteSetup();
            }
        });
    } else {
        doCompleteSetup();
    }
}

QFuture<void> Connection::Private::ensureHomeserver(const QString& userId,
                                                    const LoginFlowType& flowType)
{
    QPromise<void> promise;
    auto result = promise.future();
    promise.start();
    if (data->baseUrl().isValid() && (flowType.isEmpty() || supportsLoginFlow(flowType))) {
        promise.finish(); // Perfect, we're already good to go
    } else if (userId.startsWith(u'@') && userId.indexOf(u':') != -1) {
        // Try to ascertain the homeserver URL and flows
        q->resolveServer(userId);
        if (!flowType.isEmpty())
            QtFuture::connect(q, &Connection::loginFlowsChanged)
                .then([this, flowType, p = std::move(promise)]() mutable {
                    if (supportsLoginFlow(flowType))
                        p.finish();
                    else // Leave the promise unfinished and emit the error
                        emit q->loginError(tr("Unsupported login flow"),
                                           tr("The homeserver at %1 does not support"
                                              " login flows of type '%2'")
                                               .arg(data->baseUrl().toDisplayString(), flowType));
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
        d->syncJob.abandon();

    d->logoutJob = callApi<LogoutJob>();
    Q_ASSERT(!isLoggedIn()); // Because d->logoutJob is running
    emit stateChanged();

    QFutureInterface<void> p;
    p.reportStarted();
    connect(d->logoutJob.get(), &BaseJob::finished, this, [this, wasSyncing, p]() mutable {
        if (d->logoutJob->status().good()
            || d->logoutJob->error() == BaseJob::Unauthorised
            || d->logoutJob->error() == BaseJob::ContentAccessError) {
            if (d->syncLoopConnection)
                disconnect(d->syncLoopConnection);
            SettingsGroup("Accounts"_L1).remove(userId());
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
    d->syncJob = callApi<SyncJob>(BackgroundRequest, d->data->lastEvent(), filter, timeout)
                     .then(this, &Connection::onSyncSuccess, [this](const SyncJob *job) {
        // SyncJob persists with retries on transient errors; if it fails,
        // there's likely something serious enough to stop the loop.
        d->lastSyncSuccessful = false;
        emit isOnlineChanged();
        stopSync();
        if (job->error() == BaseJob::Unauthorised) {
            qCWarning(SYNCJOB)
                << "Sync job failed with Unauthorised - login expired?";
            emit loginError(job->errorString(), job->rawDataSample());
        } else
            emit syncError(job->errorString(), job->rawDataSample());
    });
    connect(d->syncJob, &SyncJob::retryScheduled, this,
            [this](int retriesTaken, int nextInMilliseconds) {
        d->lastSyncSuccessful = false;
        emit isOnlineChanged();
        emit networkError(d->syncJob->errorString(), d->syncJob->rawDataSample(), retriesTaken,
                          nextInMilliseconds);
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

void Connection::onSyncSuccess(SyncJob *syncJob)
{
    if (d->cryptoMachine) {
        auto syncChangesResult =
            (*d->cryptoMachine)->receive_sync_changes(bytesToRust(syncJob->rawData()));
        if (syncChangesResult->has_error()) {
            return;
        } else {
            auto syncChanges = syncChangesResult->value();

            if (syncChanges->secrets_received()) {
                emit backupFinished(BackupResult::Success);
            }

            if (syncChanges->self_verified()) {
                emit ownSessionVerified();
            }

            for (const auto &session : syncChanges->received_done_events()) {
                emit receivedVerificationDone(stringFromRust(session));
            }

            for (const auto &key : syncChanges->keys()) {
                if (const auto &r = room(stringFromRust(key.room_id()))) {
                    r->newMegolmSession(stringFromRust(key.session_id()));
                }
            }

            for (auto &session : syncChanges->sessions()) {
                auto keyVerificationSession =
                    new KeyVerificationSession(stringFromRust(session.remote_user_id()),
                                               stringFromRust(session.verification_id()),
                                               stringFromRust(session.remote_device_id()), this);
                emit newKeyVerificationSession(keyVerificationSession);
            }
        }
        emit allPrivateCSKeysAvailableChanged();
        emit isBackupDecryptionKeyAvailableChanged();
    }
    processSyncData(syncJob->takeData());

    if (d->cryptoMachine) {
        if ((*d->cryptoMachine)->has_pending_backup_key()) {
            d->initializeExistingBackup();
            importFromBackup();
        }

        if ((*d->cryptoMachine)->has_initialized_backup()) {
            const auto &requestResult = (*d->cryptoMachine)->backup_keys();
            if (!requestResult->has_error() && requestResult->has_request()
                && !d->isUploadingKeysToBackup) {
                d->isUploadingKeysToBackup = true;
                auto request = requestResult->value();
                callApi<PutRoomKeysJob>(stringFromRust(request->version()),
                                        fromRustJson<QHash<RoomId, RoomKeyBackup>>(request->rooms()))
                    .onResult([this, transaction_id = request->transaction_id()](const auto &job) {
                    d->isUploadingKeysToBackup = false;
                    (*d->cryptoMachine)
                        ->mark_keys_backup_as_sent(bytesToRust(job->rawData()), transaction_id);
                });
            }
        }
    }

    d->syncJob.clear();
    d->lastSyncSuccessful = true;
    emit isOnlineChanged();
    emit syncDone();
}

void Connection::processSyncData(SyncData&& data, bool fromCache)
{
    d->data->setLastEvent(data.nextBatch());
    d->consumeRoomData(data.takeRoomData(), fromCache);
    d->consumeAccountData(data.takeAccountData());
    d->consumePresenceData(data.takePresenceData());

    Q_UNUSED(std::move(data)) // Tell static analysers `data` is consumed now

    d->processOutgoingRequests();
}

void Connection::Private::processOutgoingRequests()
{
    if (!cryptoMachine) {
        return;
    }

    if (isHandlingOutgoing) {
        return;
    }
    const auto requestsResult = (*cryptoMachine)->outgoing_requests();
    if (requestsResult->has_error()) {
        return;
    }
    const auto requests = requestsResult->value();
    if (requests.empty()) {
        return;
    }
    QList<QFuture<void>> futures;
    for (const auto &request : requests) {
        const auto id = stringFromRust(request.id());
        const auto type = request.request_type();
        switch (type) {
            case OutgoingRequestType::KeysUpload: {
                const auto deviceKeys = jsonFromRust(request.keys_upload_device_keys());
                futures.append(QFuture<void>(
                    q->callApi<UploadKeysJob>(
                         deviceKeys.isEmpty() ? std::nullopt
                                              : std::optional(fromJson<DeviceKeys>(deviceKeys)),
                         fromRustJson<OneTimeKeys>(request.keys_upload_one_time_keys()),
                         fromRustJson<OneTimeKeys>(request.keys_upload_fallback_keys()))
                        .then([id, this](const auto& job) {
                    (*cryptoMachine)
                        ->mark_keys_upload_as_sent(bytesToRust(job->rawData()), stringToRust(id));
                })));
                break;
            }
            case OutgoingRequestType::KeysQuery: {
                const auto timeout = request.keys_query_timeout();
                futures.append(QFuture<void>(
                    q->callApi<QueryKeysJob>(fromRustJson<QHash<UserId, QStringList>>(
                                                 request.keys_query_device_keys()),
                                             timeout > 0 ? timeout : std::optional<int>())
                        .then([id, this](const auto& job) {
                    emit q->finishedQueryingKeys();
                    (*cryptoMachine)
                        ->mark_keys_query_as_sent(bytesToRust(job->rawData()), stringToRust(id));
                })));
                break;
            }
            case OutgoingRequestType::KeysClaim: {
                const auto timeout = request.keys_claim_timeout();
                futures.append(QFuture<void>(
                    q->callApi<ClaimKeysJob>(fromRustJson<QHash<UserId, QHash<QString, QString>>>(
                                                 request.keys_claim_one_time_keys()),
                                             timeout > 0 ? timeout : std::optional<int>())
                        .then([id, this](const auto& job) {
                    (*cryptoMachine)
                        ->mark_keys_claim_as_sent(bytesToRust(job->rawData()), stringToRust(id));
                })));
                break;
            }
            case OutgoingRequestType::ToDevice: {
                futures.append(
                    QFuture<void>(q->callApi<SendToDeviceJob>(
                                       stringFromRust(request.to_device_event_type()),
                                       stringFromRust(request.to_device_txn_id()),
                                       fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                           request.to_device_messages()))
                                      .then([id, this](const auto& job) {
                    (*cryptoMachine)
                        ->mark_to_device_as_sent(bytesToRust(job->rawData()), stringToRust(id));
                })));
                break;
            }
            case OutgoingRequestType::SignatureUpload: {
                futures.append(
                    QFuture<void>(q->callApi<UploadCrossSigningSignaturesJob>(
                                       fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                           request.upload_signature_signed_keys()))
                                      .then([this, id](const auto& job) {
                    (*cryptoMachine)
                        ->mark_signature_upload_as_sent(bytesToRust(job->rawData()),
                                                        stringToRust(id));
                })));
                break;
            }
            case OutgoingRequestType::RoomMessage: {
                futures.append(
                    q->room(stringFromRust(request.room_msg_room_id()))
                        ->post(loadEvent<RoomEvent>(stringFromRust(request.room_msg_matrix_type()),
                                                    jsonFromRust(request.room_msg_content())))
                        .whenMerged()
                        .then([this, id](const RoomEvent& targetEvt) {
                    (*cryptoMachine)
                        ->mark_room_message_as_sent(stringToRust(targetEvt.id()), stringToRust(id));
                }));
                break;
            }
        }
    }

    QtFuture::whenAll(futures.begin(), futures.end()).then([this](const auto &) {
        isHandlingOutgoing = false;
        processOutgoingRequests();
    });
    isHandlingOutgoing = true;
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
                [r, rd = std::move(roomData), fromCache, this] () mutable {
                    r->updateData(std::move(rd), fromCache);
                    if (cryptoMachine && r->usesEncryption()) {
                        (*cryptoMachine)->update_tracked_users(stringsToRust(r->memberIds()));
                    }
                },
                Qt::QueuedConnection);
        }
    }
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

void Connection::Private::consumeAccountData(Events&& accountDataEvents)
{
    // After running this loop, the account data events not saved in
    // accountData (see the end of the loop body) are auto-cleaned away
    for (auto&& eventPtr: accountDataEvents) {
        switchOnType(*eventPtr,
            [this](const DirectChatEvent& dce) {
                // https://github.com/quotient-im/libQuotient/wiki/Handling-direct-chat-events
                const auto& usersToDCs = dce.usersToDirectChats();
                const DirectChatsMap remoteRemovals =
                    remove_if(directChats, [&usersToDCs, this](const User* u, const QString& rId) {
                        const auto removed = !(usersToDCs.contains(u->id(), rId)
                                               || dcLocalAdditions.contains(u, rId));
                        if (removed)
                            qCDebug(MAIN) << rId << "is no more a direct chat with" << u->id();
                        return removed;
                    });
                remove_if(directChatMemberIds,
                          [&remoteRemovals, this](const QString& rId, const QString& mId) {
                              return remoteRemovals.contains(q->user(mId), rId);
                          });
                // Remove from dcLocalRemovals what the server already has.
                map_subtract(dcLocalRemovals, remoteRemovals);

                DirectChatsMap remoteAdditions;
                for (const auto& [uId, rId] : usersToDCs.asKeyValueRange()) {
                    if (const auto* const u = q->user(uId)) {
                        if (!directChats.contains(u, rId) && !dcLocalRemovals.contains(u, rId)) {
                            Q_ASSERT(!directChatMemberIds.contains(rId, uId));
                            remoteAdditions.insert(u, rId);
                            directChats.insert(u, rId);
                            directChatMemberIds.insert(rId, uId);
                            qCDebug(MAIN) << "Marked room" << rId << "as a direct chat with" << uId;
                        }
                    } else
                        qCWarning(MAIN) << "Couldn't get a user object for" << uId;
                }
                // Remove from dcLocalAdditions what the server already has.
                map_subtract(dcLocalAdditions, remoteAdditions);
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
        q->callApi<SetAccountDataJob>(data->userId(), u"m.direct"_s, toJson(directChats));
        dcLocalAdditions.clear();
        dcLocalRemovals.clear();
    }
}

void Connection::Private::consumePresenceData(Events&& presenceData)
{
    // To be implemented
}

void Connection::stopSync()
{
    // If there's a sync loop, break it
    disconnect(d->syncLoopConnection);
    if (d->syncJob) // If there's an ongoing sync job, stop it too
    {
        if (d->syncJob->status().code == BaseJob::Pending)
            d->syncJob.abandon();
        d->syncJob.clear();
    }
}

QString Connection::nextBatchToken() const { return d->data->lastEvent(); }

JobHandle<JoinRoomJob> Connection::joinRoom(const QString& roomAlias, const QStringList& serverNames)
{
    // Upon completion, ensure a room object is created in case it hasn't come with a sync yet.
    // If the room object is not there, provideRoom() will create it in Join state. Using
    // the continuation ensures that the room is provided before any client connections.
    return callApi<JoinRoomJob>(roomAlias, serverNames, serverNames)
        .then([this](const QString& roomId) { provideRoom(roomId, JoinState::Join); });
}

QFuture<Room*> Connection::joinAndGetRoom(const QString& roomAlias, const QStringList& serverNames)
{
    return callApi<JoinRoomJob>(roomAlias, serverNames, serverNames)
        .then([this](const QString& roomId) { return provideRoom(roomId, JoinState::Join); });
}

QFuture<Room *> Connection::waitForNewRoom(const QString &roomId)
{
    if (auto *newRoom = room(roomId))
        return QtFuture::makeReadyValueFuture(newRoom);

    QPromise<Room *> promise;
    auto ft = promise.future();
    connectUntil(this, &Connection::loadedRoomState, this,
                 [roomId, p = std::move(promise)](Room *newRoom) mutable {
        if (newRoom->id() == roomId) {
            p.addResult(newRoom);
            p.finish();
            return true;
        }
        return false;
    });
    return ft;
}

JobHandle<LeaveRoomJob> Connection::leaveRoom(Room* room)
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
               qPrintable(u'\'' % mediaId % "' doesn't look like 'serverName/localMediaId'"_L1));
    return idParts;
}

QUrl Connection::makeMediaUrl(QUrl mxcUrl) const
{
    Q_ASSERT(mxcUrl.scheme() == "mxc"_L1);
    QUrlQuery q(mxcUrl.query());
    q.removeAllQueryItems(u"user_id"_s);
    q.addQueryItem(u"user_id"_s, userId());
    mxcUrl.setQuery(q);
    return mxcUrl;
}

JobHandle<MediaThumbnailJob> Connection::getThumbnail(const QString& mediaId, QSize requestedSize,
                                                      RunningPolicy policy)
{
    auto idParts = splitMediaId(mediaId);
    return callApi<MediaThumbnailJob>(policy, idParts.front(), idParts.back(),
                                      requestedSize);
}

JobHandle<MediaThumbnailJob> Connection::getThumbnail(const QUrl& url, QSize requestedSize,
                                                      RunningPolicy policy)
{
    return getThumbnail(url.authority() + url.path(), requestedSize, policy);
}

JobHandle<MediaThumbnailJob> Connection::getThumbnail(const QUrl& url, int requestedWidth,
                                                      int requestedHeight, RunningPolicy policy)
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
            return {};
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

JobHandle<DownloadFileJob> Connection::downloadFile(const QUrl& url, const QString& localFilename)
{
    auto mediaId = url.authority() + url.path();
    auto idParts = splitMediaId(mediaId);
    return callApi<DownloadFileJob>(idParts.front(), idParts.back(), localFilename);
}

JobHandle<DownloadFileJob> Connection::downloadFile(const QUrl& url,
                                                    const EncryptedFileMetadata& fileMetadata,
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
    return createRoom(visibility, alias, name, topic, std::move(invites), presetName, roomVersion,
                      isDirect, initialState, {}, invite3pids, creationContent);
}

JobHandle<CreateRoomJob> Connection::createRoom(
    RoomVisibility visibility, const QString &alias, const QString &name, const QString &topic,
    QStringList invites, const QString &presetName, const QString &roomVersion, bool isDirect,
    const QVector<CreateRoomJob::StateEvent> &initialState, const QStringList &additionalCreators,
    const QVector<Invite3pid> &invite3pids, QJsonObject creationContent)
{
    invites.removeOne(userId()); // The creator is by definition in the room
    if (!additionalCreators.empty()) {
        auto creators = creationContent.take("additional_creators"_L1).toArray();
        for (const auto &ac : additionalCreators)
            if (!creators.contains(ac))
                creators.append(ac);
        creationContent.insert("additional_creators"_L1, creators);
    }
    return callApi<CreateRoomJob>(visibility == PublishRoom ? u"public"_s : u"private"_s,
                                  alias, name, topic, invites, invite3pids, roomVersion,
                                  creationContent, initialState, presetName, isDirect)
        .then(this, [this, invites, isDirect](const QString& roomId) {
            auto* room = provideRoom(roomId, JoinState::Join);
            if (QUO_ALARM_X(!room, "Failed to create a room object locally"))
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
    if (QUO_ALARM_X(!u, u"Couldn't get a user object for" % otherUserId))
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
            return QtFuture::makeReadyValueFuture(r);
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

    return createRoom(UnpublishRoom, {}, name, topic, { userId }, u"trusted_private_chat"_s, {},
                      true, initialStateEvents)
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

QVector<GetLoginFlowsJob::LoginFlow> Connection::loginFlows() const
{
    return d->loginFlows;
}

std::optional<LoginFlow> Connection::getLoginFlow(const QString& flowType) const
{
    if (auto it = std::ranges::find(d->loginFlows, flowType, &LoginFlow::type);
        it != d->loginFlows.cend())
        return *it;
    return std::nullopt;
}

bool Connection::supportsPasswordAuth() const
{
    if (auto ssoFlow = getLoginFlow(LoginFlowTypes::SSO);
        ssoFlow && ssoFlow->delegatedOidcCompatibility)
        return false; // See MSC3824
    return d->supportsLoginFlow(LoginFlowTypes::Password);
}

bool Connection::supportsSso() const
{
    return d->supportsLoginFlow(LoginFlowTypes::SSO);
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
    return d->userAvatarMap.try_emplace(mediaId, this, avatarUrl).first->second;
}

QString Connection::deviceId() const { return d->data->deviceId(); }

QByteArray Connection::accessToken() const
{
    // The logout job needs access token to do its job; so the token is
    // kept inside d->data but no more exposed to the outside world.
    return isJobPending(d->logoutJob) ? QByteArray() : d->data->accessToken();
}

bool Connection::isLoggedIn() const { return !accessToken().isEmpty(); }

bool Connection::isOnline() const { return d->lastSyncSuccessful; }

SyncJob* Connection::syncJob() const { return d->syncJob; }

int Connection::millisToReconnect() const
{
    return d->syncJob ? d->syncJob->millisToRetry() : 0;
}

QVector<Room*> Connection::allRooms() const
{
    QVector<Room*> result;
    result.resize(d->roomMap.size());
    std::ranges::copy(d->roomMap, result.begin());
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
    return static_cast<int>(std::ranges::count_if(d->roomMap, [joinStates](const Room* r) {
        return joinStates.testFlag(r->joinState());
    }));
}

bool Connection::hasAccountData(const QString& type) const
{
    return d->accountData.contains(type);
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
    // TODO: use a structured binding once https://github.com/llvm/llvm-project/issues/115137 is done
    for (auto&& p : result.asKeyValueRange()) {
        std::ranges::sort(p.second, {}, [tag=p.first](const Room* r) { return r->tag(tag); });
    }
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
    std::ranges::copy_if(d->roomMap, std::back_inserter(rooms),
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
        removals = remove_if(d->directChats, [&roomId](auto, auto rId) { return rId == roomId; });
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

    QJsonObject rootObj{ { u"cache_version"_s,
                           QJsonObject{ { u"major"_s, SyncData::cacheVersion().first },
                                        { u"minor"_s, SyncData::cacheVersion().second } } } };
    {
        QJsonObject roomsJson;
        QJsonObject inviteRoomsJson;
        for (const auto* r: std::as_const(d->roomMap)) {
            if (r->joinState() == JoinState::Leave)
                continue;
            (r->joinState() == JoinState::Invite ? inviteRoomsJson : roomsJson)
                .insert(r->id(), QJsonObject{ { u"$ref"_s, SyncData::fileNameForRoom(r->id()) } });
        }

        QJsonObject roomObj;
        if (!roomsJson.isEmpty())
            roomObj.insert("join"_L1, roomsJson);
        if (!inviteRoomsJson.isEmpty())
            roomObj.insert("invite"_L1, inviteRoomsJson);

        rootObj.insert("next_batch"_L1, d->data->lastEvent());
        rootObj.insert("rooms"_L1, roomObj);
    }
    {
        QJsonArray accountDataEvents{ Event::basicJson(DirectChatEvent::TypeId,
                                                       toJson(d->directChats)) };
        for (const auto& e : d->accountData)
            accountDataEvents.append(Event::basicJson(e.first, e.second->contentJson()));

        rootObj.insert("account_data"_L1, QJsonObject{ { u"events"_s, accountDataEvents } });
    }

    const auto data =
        d->cacheToBinary ? QCborValue::fromJsonValue(rootObj).toCbor()
                         : QJsonDocument(rootObj).toJson(QJsonDocument::Compact);
    qCDebug(PROFILER).noquote() << "Cache for" << objectName() << "generated in" << et;

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
    processSyncData(std::move(sync), true);
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

void Connection::run(BaseJob* job, RunningPolicy runningPolicy)
{
    if (job) {
        // Reparent to protect from #397, #398 and to prevent BaseJob* from being
        // garbage-collected if made by or returned to QML/JavaScript.
        job->setParent(this);
        connect(job, &BaseJob::failure, this, &Connection::requestFailed);
        job->initiate(d->data.get(), runningPolicy & BackgroundRequest);
    }
}

void Connection::getTurnServers()
{
    auto job = callApi<GetTurnServerJob>();
    connect(job, &GetTurnServerJob::success, this,
            [this,job] { emit turnServersChanged(job->data()); });
}

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
        for (const auto& [v, isStable] : d->capabilities.roomVersions->available.asKeyValueRange())
            if (isStable == SupportedRoomVersion::StableTag)
                l.push_back(v);
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

QVector<Connection::SupportedRoomVersion> Connection::availableRoomVersions() const
{
    if (!d->capabilities.roomVersions)
        return {};

    // Can't stuff QKeyValueRange in a std:: view directly because it's not move-assignable and
    // most views require that - using std::views::all to go around this
    const auto allVersions = d->capabilities.roomVersions->available.asKeyValueRange();
    auto result =
        rangeTo<QVector>(std::views::all(allVersions) | std::views::transform([](const auto& p) {
                             return SupportedRoomVersion{ p.first, p.second };
                         }));
    // Put stable versions over unstable
    std::ranges::sort(result, [](const SupportedRoomVersion& v1, const SupportedRoomVersion& v2) {
        if (const auto stable1 = v1.isStable(), stable2 = v2.isStable(); stable1 != stable2)
            return stable1 && !stable2; // Put all stable versions over unstable
        // For two versions with the same stability, if both versions are numeric order them as
        // numbers, otherwise compare strings.
        bool ok1 = false, ok2 = false;
        const auto vNum1 = v1.id.toFloat(&ok1);
        const auto vNum2 = v2.id.toFloat(&ok2);
        return ok1 && ok2 ? vNum1 < vNum2 : v1.id < v2.id;
    });
    return result;
}


QJsonObject Connection::decryptNotification(const QJsonObject& notification)
{
    if (auto r = room(notification[RoomIdKey].toString()))
        if (auto event =
                loadEvent<EncryptedEvent>(notification["event"_L1].toObject()))
            if (const auto decrypted = r->decryptMessage(*event))
                return decrypted->fullJson();
    return {};
}

Quotient::KeyVerificationSession* Connection::startKeyVerificationSession(const QString& userId,
                                                                const QString& deviceId)
{
    auto session = KeyVerificationSession::requestDeviceVerification(userId, deviceId, this);
    Q_EMIT newKeyVerificationSession(session);
    return session;
}

Connection* Connection::makeMockConnection(const QString& mxId, bool enableEncryption)
{
    auto* c = new Connection;
    c->enableEncryption(enableEncryption);
    c->d->completeSetup(mxId);
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
    auto requestResult = (*d->cryptoMachine)->request_self_verification();
    if (requestResult->has_error()) {
        return;
    }

    auto request = requestResult->value();
    callApi<SendToDeviceJob>(stringFromRust(request->to_device_event_type()),
                             stringFromRust(request->to_device_txn_id()),
                             fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                 request->to_device_messages()));
    Q_EMIT newKeyVerificationSession(
        KeyVerificationSession::selfVerification(stringFromRust(request->verification_id()), this));
}

bool Connection::allSessionsSelfVerified(const QString& userId) const
{
    return (*d->cryptoMachine)->all_sessions_verified(stringToRust(userId));
}

void Connection::Private::runShareKey(Room* room, std::function<void()> then)
{
    auto historyVisibilityEvent = room->currentState().get("m.room.history_visibility"_L1);
    uint8_t visibility = 0;
    if (historyVisibilityEvent) {
        auto visibilityString = historyVisibilityEvent->contentJson()["history_visibility"_L1].toString();
        if (visibilityString == u"joined"_s) {
            visibility = 1;
        } else if (visibilityString == u"shared"_s) {
            visibility = 2;
        } else if (visibilityString == u"world_readable"_s) {
            visibility = 3;
        }
    }

    auto ids = stringsToRust(room->joinedMemberIds());

    if (visibility == 0 || visibility == 3) {
        room->currentState().queryAll([&ids](const RoomMemberEvent& e) {
            if (e.isInvite()) {
                ids.push_back(stringToRust(e.stateKey()));
            }
        });
    }

    auto sendKeys = [then, this, room, ids, visibility](){
        auto requestsResult =
        (*cryptoMachine)->share_room_key(stringToRust(room->id()), ids, false, visibility);
        if (requestsResult->has_error()) {
            // Not running then()
            Q_EMIT q->shareRoomKeyDone();
            return;
        }
        auto requests = requestsResult->value();
        if (requests.empty()) {
            then();
            Q_EMIT q->shareRoomKeyDone();
            return;
        }
        QList<QFuture<void>> futures;
        for (const auto& request : requests) {
            auto txnId = request.txn_id();

            futures.append(QFuture<void>(q->callApi<SendToDeviceJob>(stringFromRust(request.event_type()), stringFromRust(txnId),
                                        fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                            request.messages()))
            .then([this, txnId, then](const auto& sendJob) {
                (*cryptoMachine)->mark_to_device_as_sent(bytesToRust(sendJob->rawData()), txnId);
            })));
        }
        QtFuture::whenAll(futures.begin(), futures.end()).then([this, then](const auto &) {
            then();
            Q_EMIT q->shareRoomKeyDone();
        });
    };

    auto missingResult = (*cryptoMachine)->get_missing_sessions(ids);
    if (missingResult->has_error()) {
        // Not running then()
        Q_EMIT q->shareRoomKeyDone();
        return;
    }
    if (!missingResult->has_request()) {
        sendKeys();
        return;
    }
    auto missing = missingResult->request();
    auto missingId = missing->id();
    q->callApi<ClaimKeysJob>(
         fromRustJson<QHash<UserId, QHash<QString, QString>>>(missing->one_time_keys()))
        .then([this, missingId, ids, then, sendKeys](const auto& claimJob) mutable {
        (*cryptoMachine)->mark_keys_claim_as_sent(bytesToRust(claimJob->rawData()), missingId);
        sendKeys();
    });
};

void Connection::Private::startKeyShare()
{
    if (keyShareQueue.isEmpty()) {
        return;
    }
    keyShareRunning = true;

    connect(q, &Connection::shareRoomKeyDone, q, [this] {
        keyShareRunning = false;
        QMetaObject::invokeMethod(q, [this] {
            startKeyShare();
        }, Qt::QueuedConnection);
    }, Qt::SingleShotConnection);
    auto [room, then] = keyShareQueue.dequeue();
    runShareKey(room, then);
}

void Connection::shareRoomKey(Room* room, std::function<void()> then)
{
    d->keyShareQueue.enqueue({room, then});
    if (!d->keyShareRunning) {
        d->startKeyShare();
    }
}

QString Connection::encryptRoomEvent(Room* room, const QByteArray& content, const QString& type)
{
    auto result = (*d->cryptoMachine)->encrypt_room_event(stringToRust(room->id()), bytesToRust(content), stringToRust(type));
    if (result->has_error()) {
        return {};
    }
    return stringFromRust(result->value());
}

QString Connection::decryptRoomEvent(Room* room, const QByteArray& event)
{
    if (!d->cryptoMachine) {
        return {};
    }

    auto result = (*d->cryptoMachine)->decrypt_room_event(stringToRust(room->id()), bytesToRust(event));
    if (result->has_error()) {
        return {};
    }
    return stringFromRust(result->value());
}

void Connection::Private::acceptKeyVerification(KeyVerificationSession* session)
{
    auto outgoingResult = (*cryptoMachine)->accept_verification(stringToRust(session->remoteUser()), stringToRust(session->verificationId()));
    if (outgoingResult->has_error()) {
        return;
    }
    auto outgoing = outgoingResult->value();
    if (!session->room()) {
        q->callApi<SendToDeviceJob>(stringFromRust(outgoing->to_device_event_type()),
                                    stringFromRust(outgoing->to_device_txn_id()),
                                    fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                        outgoing->to_device_messages()));
    } else {
        auto json = jsonFromRust(outgoing->in_room_content());
        auto transactionId = stringFromRust(outgoing->in_room_txn_id());
        session->room()->postJson(KeyVerificationReadyEvent::TypeId, json);
    }
    session->setState(keyVerificationSessionState(session));
    session->setSasState(sasState(session));
}

void Connection::Private::startKeyVerification(KeyVerificationSession* session)
{
    auto startSasResult = (*cryptoMachine)->start_sas(stringToRust(session->remoteUser()), stringToRust(session->verificationId()));
    if (startSasResult->has_error()) {
        return;
    }

    auto startSas = startSasResult->value();
    if (!session->room()) {
        q->callApi<SendToDeviceJob>(stringFromRust(startSas->to_device_event_type()),
                                    stringFromRust(startSas->to_device_txn_id()),
                                    fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                        startSas->to_device_messages()));
    } else {
        auto json = jsonFromRust(startSas->in_room_content());
        auto transactionId = stringFromRust(startSas->in_room_txn_id());
        session->room()->postJson(KeyVerificationStartEvent::TypeId, json);
    }
}

void Connection::Private::confirmKeyVerification(KeyVerificationSession* session)
{
    auto requestsResult = (*cryptoMachine)->confirm_verification(stringToRust(session->remoteUser()), stringToRust(session->verificationId()));
    if (requestsResult->has_error()) {
        return;
    }
    auto requests = requestsResult->value();
    for (const auto& request : requests->verification_requests()) {
        if (!session->room()) {
            const auto& type = stringFromRust(request.to_device_event_type());
            q->callApi<SendToDeviceJob>(type, stringFromRust(request.to_device_txn_id()),
                                        fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                            request.to_device_messages()));
            if (type == KeyVerificationDoneEvent::TypeId) {
                session->setState(KeyVerificationSession::DONE);
            }
            if (type == KeyVerificationCancelEvent::TypeId) {
                session->setState(KeyVerificationSession::CANCELLED);
            }
        } else {
            const auto& type = stringFromRust(request.in_room_event_type());
            session->room()->postJson(stringFromRust(request.in_room_event_type()), jsonFromRust(request.in_room_content()));
            if (type == KeyVerificationDoneEvent::TypeId) {
                session->setState(KeyVerificationSession::DONE);
            }
            if (type == KeyVerificationCancelEvent::TypeId) {
                session->setState(KeyVerificationSession::CANCELLED);
            }
        }
    }
    if (requests->has_signature_request()) {
        q->callApi<UploadCrossSigningSignaturesJob>(
            fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                requests->signature_request_content()));
    }
    session->setSasState(sasState(session));
}

void Connection::Private::cancelKeyVerification(KeyVerificationSession* session)
{
    auto outgoingResult = (*cryptoMachine)->cancel_verification(stringToRust(session->remoteUser()), stringToRust(session->verificationId()));
    if (outgoingResult->has_error()) {
        return;
    }
    auto outgoing = outgoingResult->value();
    if (!session->room()) {
        q->callApi<SendToDeviceJob>(stringFromRust(outgoing->to_device_event_type()),
                                    stringFromRust(outgoing->to_device_txn_id()),
                                    fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                        outgoing->to_device_messages()));
    } else {
        auto json = jsonFromRust(outgoing->in_room_content());
        auto transactionId = stringFromRust(outgoing->in_room_txn_id());
        session->room()->postJson(KeyVerificationCancelEvent::TypeId, json);
    }
    session->setState(keyVerificationSessionState(session));
    session->setSasState(sasState(session));
}


void Connection::Private::acceptSas(KeyVerificationSession* session)
{
    const auto& requestResult = (*cryptoMachine)->accept_sas(stringToRust(session->remoteUser()), stringToRust(session->verificationId()));
    if (requestResult->has_error()) {
        return;
    }
    const auto& request = requestResult->value();
    if (!session->room()) {
        q->callApi<SendToDeviceJob>(stringFromRust(request->to_device_event_type()),
                                    stringFromRust(request->to_device_txn_id()),
                                    fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                        request->to_device_messages()));
    } else {
        session->room()->postJson(KeyVerificationAcceptEvent::TypeId, jsonFromRust(request->in_room_content()));
    }
    session->setSasState(sasState(session));
}


KeyVerificationSession::State Connection::Private::keyVerificationSessionState(KeyVerificationSession* session)
{
    auto result = (*cryptoMachine)->verification_get_state(stringToRust(session->remoteUser()), stringToRust(session->verificationId()));
    if (result->has_error()) {
        return {};
    }
    return (KeyVerificationSession::State) result->value();
}

KeyVerificationSession::SasState Connection::Private::sasState(KeyVerificationSession* session)
{
    auto result = (*cryptoMachine)->sas_get_state(stringToRust(session->remoteUser()), stringToRust(session->verificationId()));
    if (result->has_error()) {
        return  {};
    }
    return (KeyVerificationSession::SasState) result->value();
}

QList<std::pair<QString, QString>> Connection::Private::keyVerificationSasEmoji(KeyVerificationSession* session)
{
    auto result = (*cryptoMachine)->sas_emoji(stringToRust(session->remoteUser()), stringToRust(session->verificationId()));
    if (result->has_error()) {
        return {};
    }

    auto e = result->value();

    QList<std::pair<QString, QString>> out;

    for (const auto& emoji : e) {
        out += {stringFromRust(emoji.symbol()), stringFromRust(emoji.description())};
    }
    return out;
}

void Connection::Private::requestDeviceVerification(KeyVerificationSession* session)
{
    auto requestResult = (*cryptoMachine)
                             ->request_device_verification(stringToRust(session->remoteUser()),
                                                           stringToRust(session->remoteDeviceId()));
    if (requestResult->has_error()) {
        return;
    }
    auto request = requestResult->value();
    q->callApi<SendToDeviceJob>(stringFromRust(request->to_device_event_type()),
                                stringFromRust(request->to_device_txn_id()),
                                fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(
                                    request->to_device_messages()));
    session->setVerificationId(stringFromRust(request->verification_id()));
}

bool Connection::isVerifiedEvent(const QString& eventId, Room* room)
{
    if (!d->cryptoMachine) {
        return false;
    }

    if (eventId.isEmpty()) {
        return false;
    }

    const auto timelineIt = room->findInTimeline(eventId);
    if (timelineIt == room->historyEdge()) {
        return false;
    }

    auto event = timelineIt->get();

    QJsonObject json;

    if (event->is<EncryptedEvent>()) {
        json = event->fullJson();
    } else if (const auto& originalEvent = event->originalEvent()) {
        json = originalEvent->fullJson();
    }
    auto rustJson = bytesToRust(QJsonDocument(json).toJson(QJsonDocument::Compact));
    auto info = (*d->cryptoMachine)->get_room_event_encryption_info(rustJson, stringToRust(room->id()));
    if (info->has_error()) {
        return false;
    }
    return info->value()->is_verified();
}

Quotient::KeyVerificationSession* Connection::requestUserVerification(Room* room)
{
    auto session = KeyVerificationSession::requestUserVerification(room, this);
    emit newKeyVerificationSession(session);
    return session;
}

void Connection::Private::requestUserVerification(KeyVerificationSession* session)
{
    auto request = (*cryptoMachine)->request_user_verification_content(stringToRust(session->remoteUser()));
    if (request->has_error()) {
        return;
    }
    auto transactionId = session->room()->postJson(RoomMessageEvent::TypeId, jsonFromRust(request->value()));
    connectUntil(session->room(), &Room::pendingEventAboutToMerge, q, [this, transactionId, session](const auto &event) {
        if (event->transactionId() != transactionId) {
            return false;
        }

        auto rustSessionResult = (*cryptoMachine)->request_user_verification(stringToRust(session->remoteUser()), stringToRust(session->room()->id()), stringToRust(event->id()));
        if (rustSessionResult->has_error()) {
            return true;
        }
        session->setVerificationId(stringFromRust(rustSessionResult->value()->verification_id()));
        session->startMonitoring();
        return true;
    });
}

void Connection::receiveVerificationEvent(const QByteArray& fullJson)
{
    (*d->cryptoMachine)->receive_verification_event(bytesToRust(fullJson));
    emit verificationEventProcessed();
}

void Connection::importFromBackup()
{
    callApi<GetRoomKeysVersionCurrentJob>().then([this](const auto versionJob) {
        const auto version = versionJob->version();
        callApi<GetRoomKeysJob>(version).then([this, version](const auto keysJob) {
            auto keys = (*d->cryptoMachine)->import_from_backup(bytesToRust(keysJob->rawData()), stringToRust(version));
            for (const auto &key : keys) {
                if (const auto &r = room(stringFromRust(key.room_id()))) {
                    r->newMegolmSession(stringFromRust(key.session_id()));
                }
            }
            emit backupFinished(Success);
        }, [this] {
            emit backupFinished(Error);
        });
    }, [this] {
        emit backupFinished(Error);
    });
}

void Connection::loadFromBackup(const QString& passphrase)
{
    if ((*d->cryptoMachine)->has_initialized_backup()) {
        importFromBackup();
        return;
    }
    auto versionJob = callApi<GetRoomKeysVersionCurrentJob>();
    connect(versionJob, &BaseJob::finished, this, [this, versionJob, passphrase] {
        const auto &version = versionJob->version();
        const auto& defaultKeyEvent = accountData("m.secret_storage.default_key"_L1);
        auto defaultKey = defaultKeyEvent->contentPart<QString>("key"_L1);
        const auto keyName = "m.secret_storage.key."_L1 + defaultKey;
        const auto &storageKeyEvent = accountData(keyName);
        const auto &backupKeyEvent = accountData(u"m.megolm_backup.v1"_s);
        const auto &masterKeyEvent = accountData(u"m.cross_signing.master"_s);
        const auto &selfKeyEvent = accountData(u"m.cross_signing.self_signing"_s);
        const auto &userKeyEvent = accountData(u"m.cross_signing.user_signing"_s);

        auto request = (*d->cryptoMachine)->load_secrets(
            stringToRust(passphrase),
            stringToRust(defaultKey),
            storageKeyEvent->contentPart<QJsonObject>(u"passphrase"_s)[u"iterations"_s].toInt(),
            stringToRust(storageKeyEvent->contentPart<QJsonObject>(u"passphrase"_s)[u"salt"_s].toString()),
            stringToRust(storageKeyEvent->contentPart<QString>(u"iv"_s)),
            stringToRust(storageKeyEvent->contentPart<QString>(u"mac"_s)),
            stringToRust(backupKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"iv"_s].toString()),
            stringToRust(backupKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"ciphertext"_s].toString()),
            stringToRust(backupKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"mac"_s].toString()),

            stringToRust(masterKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"iv"_s].toString()),
            stringToRust(masterKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"ciphertext"_s].toString()),
            stringToRust(masterKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"mac"_s].toString()),

            stringToRust(selfKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"iv"_s].toString()),
            stringToRust(selfKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"ciphertext"_s].toString()),
            stringToRust(selfKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"mac"_s].toString()),

            stringToRust(userKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"iv"_s].toString()),
            stringToRust(userKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"ciphertext"_s].toString()),
            stringToRust(userKeyEvent->contentPart<QJsonObject>(u"encrypted"_s)[defaultKey][u"mac"_s].toString()),

            stringToRust(version)
        );
        if (request->is_invalid_passphrase()) {
            emit backupFinished(InvalidPassphrase);
            return;
        }
        if (request->has_error()) {
            emit backupFinished(Error);
            return;
        }

        emit allPrivateCSKeysAvailableChanged();
        emit isBackupDecryptionKeyAvailableChanged();
        callApi<UploadCrossSigningSignaturesJob>(
            fromRustJson<QHash<UserId, QHash<QString, QJsonObject>>>(request->value()));
        importFromBackup();
    });
}

void Connection::requestSecretsFromDevices()
{
    (*d->cryptoMachine)->request_secrets_from_devices();
}

void Connection::Private::initializeExistingBackup()
{
    if (isInitializingBackup) {
        return;
    }
    isInitializingBackup = true;

    q->callApi<GetRoomKeysVersionCurrentJob>().onResult([this](const auto& job) {
        (*cryptoMachine)->initialize_existing_backup(bytesToRust(job->rawData()));
    });
}

KeyImport::Error Connection::importKeys(const QString& passphrase, const QString& data)
{
    auto keysResult = (*d->cryptoMachine)->import_keys(stringToRust(passphrase), stringToRust(data));

    if (keysResult->has_error()) {
        return (KeyImport::Error) keysResult->error_code();
    }

    for (const auto& key : keysResult->value()) {
        if (const auto& room = this->room(stringFromRust(key.room_id()))) {
            room->newMegolmSession(stringFromRust(key.session_id()));
        }
    }
    return KeyImport::Success;
}

QByteArray Connection::exportKeys(const QString& passphrase)
{
    auto result = (*d->cryptoMachine)->export_keys(stringToRust(passphrase));
    if (result->has_error()) {
        return {};
    }
    return bytesFromRust(result->value());
}

bool Connection::isUserVerified(const QString& userId) const
{
    if (!d->cryptoMachine) {
        return false;
    }
    return (*d->cryptoMachine)->is_user_verified(stringToRust(userId));
}

bool Connection::isKnownE2eeCapableDevice(const QString& userId, const QString& deviceId) const
{
    return d->cryptoMachine && (*d->cryptoMachine)->is_e2ee_device(stringToRust(userId), stringToRust(deviceId));
}

bool Connection::isVerifiedDevice(const QString& userId, const QString& deviceId) const
{
    return d->cryptoMachine && (*d->cryptoMachine)->is_verified_device(stringToRust(userId), stringToRust(deviceId));
}

void monitorCallback(rust::String ourUserId, rust::String theirUserId, rust::String verificationId)
{
    emit Dispatcher::instance().sessionChanged(stringFromRust(ourUserId),
                                               stringFromRust(theirUserId),
                                               stringFromRust(verificationId));
}

void Connection::Private::monitorVerification(KeyVerificationSession *session)
{
    if (!cryptoMachine) {
        return;
    }
    (*cryptoMachine)
        ->monitor_verification(stringToRust(session->remoteUser()),
                               stringToRust(session->verificationId()), (uint64_t)monitorCallback);
}

void Connection::Private::monitorSas(KeyVerificationSession *session)
{
    if (!cryptoMachine) {
        return;
    }
    (*cryptoMachine)
        ->monitor_sas(stringToRust(session->remoteUser()), stringToRust(session->verificationId()),
                      (uint64_t)monitorCallback);
}

bool Connection::isBackupDecryptionKeyAvailable() const
{
    if (!d->cryptoMachine) {
        return false;
    }
    return (*d->cryptoMachine)->has_initialized_backup();
}

bool Connection::allPrivateCSKeysAvailable() const
{
    if (!d->cryptoMachine) {
        return false;
    }
    return (*d->cryptoMachine)->all_private_cs_keys_available();
}

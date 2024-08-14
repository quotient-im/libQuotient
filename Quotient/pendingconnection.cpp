// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "pendingconnection.h"

#include "connectiondata.h"
#include "connectionencryptiondata_p.h"
#include "logging_categories_p.h"
#include "csapi/login.h"
#include "csapi/wellknown.h"
#include "jobs/jobhandle.h"
#include "connection.h"
#include "connection_p.h"
#include "config.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>

using namespace Quotient;
using namespace Qt::Literals::StringLiterals;

using LoginFlow = GetLoginFlowsJob::LoginFlow;

//! Predefined login flows
namespace LoginFlows {
    inline const LoginFlow Password { "m.login.password"_ls };
    inline const LoginFlow SSO { "m.login.sso"_ls };
    inline const LoginFlow Token { "m.login.token"_ls };
}

// To simplify comparisons of LoginFlows

inline bool operator==(const LoginFlow& lhs, const LoginFlow& rhs)
{
    return lhs.type == rhs.type;
}

inline bool operator!=(const LoginFlow& lhs, const LoginFlow& rhs)
{
    return !(lhs == rhs);
}


class Quotient::PendingConnection::Private
{
public:
    Private(PendingConnection *qq)
        : q(qq)
        , connectionData(new ConnectionData({}))
        {}

    //! \brief Check the homeserver and resolve it if needed, before connecting
    //!
    //! A single entry for functions that need to check whether the homeserver is valid before
    //! running. Emits resolveError() if the homeserver URL is not valid and cannot be resolved
    //! from \p userId; loginError() if the homeserver is accessible but doesn't support \p flow.
    //!
    //! \param userId    fully-qualified MXID to resolve HS from
    //! \param flow      optionally, a login flow that should be supported;
    //!                  `std::nullopt`, if there are no login flow requirements
    //! \return a future that becomes ready once the homeserver is available; if the homeserver
    //!         URL is incorrect or other problems occur, the future is never resolved and is
    //!         deleted (along with associated continuations) as soon as the problem becomes
    //!         apparent
    //! \sa resolveServer, resolveError, loginError
    QFuture<void> ensureHomeserver(const QString& userId, const std::optional<LoginFlow>& flow = {});

    //! \brief Determine and set the homeserver from MXID
    //!
    //! This attempts to resolve the homeserver by requesting
    //! .well-known/matrix/client record from the server taken from the MXID
    //! serverpart. If there is no record found, the serverpart itself is
    //! attempted as the homeserver base URL; if the record is there but
    //! is malformed (e.g., the homeserver base URL cannot be found in it)
    //! resolveError() is emitted and further processing stops. Otherwise,
    //! setHomeserver is called, preparing the Connection object for the login
    //! attempt.
    //! \param mxid user Matrix ID, such as @someone:example.org
    //! \sa setHomeserver, homeserverChanged, loginFlowsChanged, resolveError
    void resolveServer(const QString& mxid);

    template <typename... LoginArgTs>
    void loginToServer(LoginArgTs&&... loginArgs);

    //! \brief Set the homeserver base URL and retrieve its login flows
    //!
    //! \sa LoginFlowsJob, loginFlows, loginFlowsChanged, homeserverChanged
    Q_INVOKABLE QFuture<QList<LoginFlow> > setHomeserver(const QUrl& baseUrl);


    BaseJob* run(BaseJob* job, RunningPolicy runningPolicy);


    //! \brief Start a job of a given type with specified arguments and policy
    //!
    //! This is a universal method to create and start a job of a type passed
    //! as a template parameter. The policy allows to fine-tune the way
    //! the job is executed - as of this writing it means a choice
    //! between "foreground" and "background".
    //!
    //! \param runningPolicy controls how the job is executed
    //! \param jobArgs arguments to the job constructor
    //!
    //! \sa BaseJob::isBackground. QNetworkRequest::BackgroundRequestAttribute
    template <typename JobT, typename... JobArgTs>
    JobHandle<JobT> callApi(RunningPolicy runningPolicy, JobArgTs&&... jobArgs)
    {
        auto job = new JobT(std::forward<JobArgTs>(jobArgs)...);
        run(job, runningPolicy);
        return job;
    }

    //! \brief Start a job of a specified type with specified arguments
    //!
    //! This is an overload that runs the job with "foreground" policy.
    template <typename JobT, typename... JobArgTs>
    JobHandle<JobT> callApi(JobArgTs&&... jobArgs)
    {
        return callApi<JobT>(ForegroundRequest, std::forward<JobArgTs>(jobArgs)...);
    }

    QKeychain::ReadPasswordJob *loadAccessTokenFromKeyChain()
    {
        qDebug() << "Reading access token from the keychain for" << connectionData->userId();
        auto job = new QKeychain::ReadPasswordJob(qAppName(), q);
        job->setKey(connectionData->userId());

        // connect(job, &QKeychain::Job::finished, q, [this, job]() {
        //     if (job->error() == QKeychain::Error::NoError) {
        //         return;
        //     }
        //
        //     // switch (job->error()) {
        //     // case QKeychain::EntryNotFound:
        //     //     Q_EMIT errorOccured(i18n("Access token wasn't found"), i18n("Maybe it was deleted?"));
        //     //     break;
        //     // case QKeychain::AccessDeniedByUser:
        //     // case QKeychain::AccessDenied:
        //     //     Q_EMIT errorOccured(i18n("Access to keychain was denied."), i18n("Please allow NeoChat to read the access token"));
        //     //     break;
        //     // case QKeychain::NoBackendAvailable:
        //     //     Q_EMIT errorOccured(i18n("No keychain available."), i18n("Please install a keychain, e.g. KWallet or GNOME keyring on Linux"));
        //     //     break;
        //     // case QKeychain::OtherError:
        //     //     Q_EMIT errorOccured(i18n("Unable to read access token"), job->errorString());
        //     //     break;
        //     // default:
        //     //     break;
        //     // }
        // });
        job->start();

        return job;
    }


    void completeSetup(const QString& mxId, bool mock);
    void saveAccessTokenToKeychain() const;

    QVector<GetLoginFlowsJob::LoginFlow> loginFlows;
    JobHandle<GetWellknownJob> resolverJob = nullptr;
    JobHandle<GetLoginFlowsJob> loginFlowsJob = nullptr;

    PendingConnection* q;

    ConnectionData* connectionData = nullptr; //TODO this should be a unique_ptr
    Connection* connection = nullptr;
    AccountRegistry* accountRegistry;
};

inline UserIdentifier makeUserIdentifier(const QString& id)
{
    return { QStringLiteral("m.id.user"), { { QStringLiteral("user"), id } } };
}

PendingConnection::PendingConnection(const QString& userId, const QString& password, const Quotient::ConnectionSettings& settings, Quotient::AccountRegistry* accountRegistry)
    : d(new Private(this))
{
    d->accountRegistry = accountRegistry;
    d->ensureHomeserver(userId, LoginFlows::Password).then([=, this] {
        d->loginToServer(LoginFlows::Password.type, makeUserIdentifier(userId),
                         password, /*token*/ QString(), settings.deviceId, settings.initialDeviceName);
    });
}

PendingConnection::PendingConnection(const QString& userId, const Quotient::ConnectionSettings& settings, Quotient::AccountRegistry* accountRegistry)
    : d(new Private(this))
{
    d->accountRegistry = accountRegistry;
    auto config = Config::instance();

    d->connectionData->setUserId(userId);
    d->connectionData->setBaseUrl(QUrl(config->load(u"Accounts"_s, userId, u"Homeserver"_s, Config::Data)));
    d->connectionData->setDeviceId(config->load(u"Accounts"_s, userId, u"DeviceId"_s, Config::Data));

    auto job = d->loadAccessTokenFromKeyChain();
    connect(job, &QKeychain::ReadPasswordJob::finished, this, [this, job](){
        d->connectionData->setToken(job->binaryData());
        d->connection = new Connection(d->connectionData);

        //TODO do this in a more unified place
        //TODO if (settings.cacheState)
        d->connection->loadState();
        connect(d->connection, &Connection::syncDone, d->connection, &Connection::saveState);
        emit ready();
        d->accountRegistry->add(d->connection);

        //TODO if (settings.sync)
        d->connection->syncLoop();
    });
}

PendingConnection *PendingConnection::loginWithPassword(const QString& userId, const QString& password, const Quotient::ConnectionSettings& settings, Quotient::AccountRegistry* accountRegistry)
{
    return new PendingConnection(userId, password, settings, accountRegistry);
}

PendingConnection *PendingConnection::restoreConnection(const QString& userId, const Quotient::ConnectionSettings& settings, Quotient::AccountRegistry* accountRegistry)
{
    return new PendingConnection(userId, settings, accountRegistry);
}

QFuture<void> PendingConnection::Private::ensureHomeserver(const QString& userId,
                                                    const std::optional<LoginFlow>& flow)
{
    QPromise<void> promise;
    auto result = promise.future();
    promise.start();
    if (connectionData->baseUrl().isValid()/* TODO && (!flow || loginFlows.contains(*flow))*/) {
        q->setObjectName(userId % u"(?)");
        promise.finish(); // Perfect, we're already good to go
    } else if (userId.startsWith(u'@') && userId.indexOf(u':') != -1) {
        // Try to ascertain the homeserver URL and flows
        q->setObjectName(userId % u"(?)");
        resolveServer(userId);
        if (flow)
            QtFuture::connect(q, &PendingConnection::loginFlowsChanged)
                .then([this, flow, p = std::move(promise)]() mutable {
                    // TODO if (loginFlows.contains(*flow))
                        p.finish();
                    // else // Leave the promise unfinished and emit the error
                    //     emit q->loginError(tr("Unsupported login flow"),
                    //                        tr("The homeserver at %1 does not support"
                    //                           " the login flow '%2'")
                    //                            .arg(connectionData->baseUrl().toDisplayString(), flow->type));
                });
        else // Any flow is fine, just wait until the homeserver is resolved
            return QFuture<void>(QtFuture::connect(q, &PendingConnection::homeserverChanged));
    } else // Leave the promise unfinished and emit the error
        emit q->resolveError(tr("Please provide the fully-qualified user id"
                                " (such as @user:example.org) so that the"
                                " homeserver could be resolved; the current"
                                " homeserver URL(%1) is not good")
                                 .arg(connectionData->baseUrl().toDisplayString()));
    return result;
}


template <typename... LoginArgTs>
void PendingConnection::Private::loginToServer(LoginArgTs&&... loginArgs)
{
    auto loginJob =
            callApi<LoginJob>(std::forward<LoginArgTs>(loginArgs)...);
    connect(loginJob, &BaseJob::success, q, [this, loginJob] {
        connectionData->setToken(loginJob->accessToken().toLatin1());
        connectionData->setDeviceId(loginJob->deviceId());
        completeSetup(loginJob->userId(), false);
        saveAccessTokenToKeychain();
        // if (encryptionData) probably not needed?
        //     encryptionData->database.clear();
    });
    connect(loginJob, &BaseJob::failure, q, [this, loginJob] {
        emit q->loginError(loginJob->errorString(), loginJob->rawDataSample());
    });
}

void PendingConnection::Private::resolveServer(const QString& mxid)
{
    resolverJob.abandon(); // The previous network request is no more relevant

    auto maybeBaseUrl = QUrl::fromUserInput(serverPart(mxid));
    maybeBaseUrl.setScheme("https"_ls); // Instead of the Qt-default "http"
    if (maybeBaseUrl.isEmpty() || !maybeBaseUrl.isValid()) {
        emit q->resolveError(tr("%1 is not a valid homeserver address")
                              .arg(maybeBaseUrl.toString()));
        return;
    }

    qCDebug(MAIN) << "Finding the server" << maybeBaseUrl.host();

    const auto& oldBaseUrl = connectionData->baseUrl();
    connectionData->setBaseUrl(maybeBaseUrl); // Temporarily set it for this one call
    resolverJob = callApi<GetWellknownJob>();
    // Make sure baseUrl is restored in any case, even an abandon, and before any further processing
    connect(resolverJob.get(), &BaseJob::finished, q,
            [this, oldBaseUrl] { connectionData->setBaseUrl(oldBaseUrl); });
    resolverJob.onResult(q, [this, maybeBaseUrl]() mutable {
        if (resolverJob->error() != BaseJob::NotFound) {
            if (!resolverJob->status().good()) {
                qCWarning(MAIN) << "Fetching .well-known file failed, FAIL_PROMPT";
                emit q->resolveError(tr("Failed resolving the homeserver"));
                return;
            }
            const QUrl baseUrl{ resolverJob->data().homeserver.baseUrl };
            if (baseUrl.isEmpty()) {
                qCWarning(MAIN) << "base_url not provided, FAIL_PROMPT";
                emit q->resolveError(tr("The homeserver base URL is not provided"));
                return;
            }
            if (!baseUrl.isValid()) {
                qCWarning(MAIN) << "base_url invalid, FAIL_ERROR";
                emit q->resolveError(tr("The homeserver base URL is invalid"));
                return;
            }
            qCInfo(MAIN) << ".well-known URL for" << maybeBaseUrl.host() << "is"
                         << baseUrl.toString();
            setHomeserver(baseUrl);
        } else {
            qCInfo(MAIN) << "No .well-known file, using" << maybeBaseUrl << "for base URL";
            setHomeserver(maybeBaseUrl);
        }
        Q_ASSERT(loginFlowsJob != nullptr); // Ensured by setHomeserver()
    });
}

QFuture<QList<LoginFlow>> PendingConnection::Private::setHomeserver(const QUrl& baseUrl)
{
    resolverJob.abandon();
    loginFlowsJob.abandon();
    loginFlows.clear();

    if (connectionData->baseUrl() != baseUrl) {
        connectionData->setBaseUrl(baseUrl);
        emit q->homeserverChanged(baseUrl);
    }

    loginFlowsJob = callApi<GetLoginFlowsJob>(BackgroundRequest).onResult([this] {
        if (loginFlowsJob->status().good())
            loginFlows = loginFlowsJob->flows();
        else
            loginFlows.clear();
        emit q->loginFlowsChanged();
    });
    return loginFlowsJob.responseFuture();
}

BaseJob* PendingConnection::Private::run(BaseJob* job, RunningPolicy runningPolicy)
{
    // Reparent to protect from #397, #398 and to prevent BaseJob* from being
    // garbage-collected if made by or returned to QML/JavaScript.
    job->setParent(q);
    //TODO maybe connect(job, &BaseJob::failure, this, &Connection::requestFailed);
    job->initiate(connectionData, runningPolicy & BackgroundRequest);
    return job;
}


void PendingConnection::Private::completeSetup(const QString& mxId, bool mock)
{
    connectionData->setUserId(mxId);
    q->setObjectName(connectionData->userId() % u'/' % connectionData->deviceId());
    qCDebug(MAIN) << "Using server" << connectionData->baseUrl().toDisplayString()
                  << "by user" << connectionData->userId()
                  << "from device" << connectionData->deviceId();

    connection = new Connection(connectionData);

    //if (settings.useEncryption) {
        if (auto&& maybeEncryptionData = _impl::ConnectionEncryptionData::setup(connection, mock)) {
            connection->d->encryptionData = std::move(*maybeEncryptionData);
        } else {
            //TODO useEncryption = false;
            //TODO emit q->encryptionChanged(false);
        }
    // } else
    //     qCInfo(E2EE) << "End-to-end encryption (E2EE) support is off for"
    //                  << q->objectName();


    //TODO if (settings.cacheState)
    connection->loadState();
    connect(connection, &Connection::syncDone, connection, &Connection::saveState);

    //TODO if (settings.sync)
    connection->syncLoop();

    if (true/*settings.remember*/) {
        auto config = Config::instance();
        config->store(u"Accounts"_s, connection->userId(), u"DeviceId"_s, connection->deviceId(), Config::Data);
        config->store(u"Accounts"_s, connection->userId(), u"Homeserver"_s, connection->homeserver().toString(), Config::Data);
    }

    emit q->ready();
    accountRegistry->add(connection);
}

PendingConnection::~PendingConnection() = default;

void PendingConnection::Private::saveAccessTokenToKeychain() const
{
    qCDebug(MAIN) << "Saving access token to keychain for" << connectionData->userId();
    auto job = new QKeychain::WritePasswordJob(qAppName());
    job->setKey(connectionData->userId());
    job->setBinaryData(connectionData->accessToken());
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

QString PendingConnection::userId() const
{
    return d->connectionData->userId();
}

Connection* PendingConnection::connection() const
{
    return d->connection;
}

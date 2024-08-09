// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "connectiondata.h"

#include "logging_categories_p.h"
#include "networkaccessmanager.h"

#include "jobs/basejob.h"

#include <QtCore/QPointer>
#include <QtCore/QTimer>

#include <array>
#include <queue>

using namespace Quotient;

class ConnectionData::Private {
public:
    explicit Private(QUrl url) : baseUrl(std::move(url))
    {
        rateLimiter.setSingleShot(true);
    }

    QUrl baseUrl;
    QByteArray accessToken;
    QString refreshToken;
    QString lastEvent;
    QString userId;
    QString deviceId;
    QStringList supportedSpecVersions;
    QString tokenEndpoint;
    QString clientId;

    mutable unsigned int txnCounter = 0;
    const qint64 txnBase = QDateTime::currentMSecsSinceEpoch();

    QString id() const { return userId + u'/' + deviceId; }

    using job_queue_t = std::queue<QPointer<BaseJob>>;
    std::array<job_queue_t, 2> jobs; // 0 - foreground, 1 - background
    QTimer rateLimiter;
};

ConnectionData::ConnectionData(QUrl baseUrl)
    : d(makeImpl<Private>(std::move(baseUrl)))
{
    // Each lambda invocation below takes no more than one job from the
    // queues (first foreground, then background) and resumes it; then
    // restarts the rate limiter timer with duration 0, effectively yielding
    // to the event loop and then resuming until both queues are empty.
    d->rateLimiter.callOnTimeout([this] {
        // TODO: Consider moving out all job->sendRequest() invocations to a dedicated thread
        d->rateLimiter.setInterval(0);
        for (auto& q : d->jobs)
            while (!q.empty()) {
                const auto job = q.front();
                q.pop();
                if (!job || job->error() == BaseJob::Abandoned)
                    continue;
                if (job->error() != BaseJob::Pending) {
                    qCCritical(MAIN)
                        << "Job" << job
                        << "is in the wrong status:" << job->status();
                    Q_ASSERT(false);
                    job->setStatus(BaseJob::Pending);
                }
                job->sendRequest();
                d->rateLimiter.start();
                return;
            }
        qCDebug(MAIN) << d->id() << "job queues are empty";
    });
}

ConnectionData::~ConnectionData()
{
    d->rateLimiter.disconnect();
    d->rateLimiter.stop();
}

void ConnectionData::submit(BaseJob* job)
{
    job->setStatus(BaseJob::Pending);
    if (!d->rateLimiter.isActive()) {
        QTimer::singleShot(0, job, &BaseJob::sendRequest);
        return;
    }
    d->jobs[size_t(job->isBackground())].emplace(job);
    qCDebug(MAIN) << job << "queued," << d->jobs.front().size() << "(fg) +"
                  << d->jobs.back().size() << "(bg) total jobs in" << d->id()
                  << "queues";
}

void ConnectionData::limitRate(std::chrono::milliseconds nextCallAfter)
{
    qCDebug(MAIN) << "Jobs for" << (d->userId + u'/' + d->deviceId)
                  << "suspended for" << nextCallAfter.count() << "ms";
    d->rateLimiter.start(nextCallAfter);
}

QByteArray ConnectionData::accessToken() const { return d->accessToken; }

QUrl ConnectionData::baseUrl() const { return d->baseUrl; }

HomeserverData ConnectionData::homeserverData() const
{
    return { d->baseUrl, d->supportedSpecVersions };
}

NetworkAccessManager* ConnectionData::nam() const
{
    return NetworkAccessManager::instance();
}

void ConnectionData::setBaseUrl(QUrl baseUrl)
{
    d->baseUrl = std::move(baseUrl);
    qCDebug(MAIN) << "updated baseUrl to" << d->baseUrl;
    if (!d->userId.isEmpty()) {
        if (d->baseUrl.isValid())
            NetworkAccessManager::addAccount(d->userId, d->baseUrl);
        else
            NetworkAccessManager::dropAccount(d->userId);
    }
}

QString ConnectionData::refreshToken() const
{
    return d->refreshToken;
}

void ConnectionData::setRefreshToken(const QString& refreshToken)
{
    qWarning() << "setting refres toke" << refreshToken;
    d->refreshToken = refreshToken;
}

void ConnectionData::setToken(QByteArray token)
{
    d->accessToken = std::move(token);
}

const QString& ConnectionData::deviceId() const { return d->deviceId; }

const QString& ConnectionData::userId() const { return d->userId; }

void ConnectionData::setDeviceId(const QString& deviceId)
{
    d->deviceId = deviceId;
}

void ConnectionData::setUserId(const QString& userId)
{
    if (d->baseUrl.isValid()) {
        if (d->userId != userId)
            NetworkAccessManager::dropAccount(d->userId);
        if (!userId.isEmpty())
            NetworkAccessManager::addAccount(userId, d->baseUrl);
    }
    d->userId = userId;
}

void ConnectionData::setSupportedSpecVersions(QStringList versions)
{
    qCInfo(MAIN).noquote() << "CS API versions:" << versions.join(u' ');
    d->supportedSpecVersions = std::move(versions);
    if (!ALARM(d->userId.isEmpty()) && !ALARM(!d->baseUrl.isValid()))
        NetworkAccessManager::updateAccountSpecVersions(d->userId, d->supportedSpecVersions);
}

QString ConnectionData::lastEvent() const { return d->lastEvent; }

void ConnectionData::setLastEvent(QString identifier)
{
    d->lastEvent = std::move(identifier);
}

QString ConnectionData::generateTxnId() const
{
    return d->deviceId + QString::number(d->txnBase)
           + QString::number(++d->txnCounter);
}

QString ConnectionData::clientId() const
{
    return d->clientId;
}

QString ConnectionData::tokenEndpoint() const
{
    return d->tokenEndpoint;
}

void ConnectionData::setClientId(const QString& clientId)
{
    d->clientId = clientId;
}

void ConnectionData::setTokenEndpoint(const QString& tokenEndpoint)
{
    d->tokenEndpoint = tokenEndpoint;
}

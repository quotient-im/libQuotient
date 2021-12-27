// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "connectiondata.h"

#include "logging.h"
#include "networkaccessmanager.h"
#include "jobs/basejob.h"

#include <QtCore/QTimer>
#include <QtCore/QPointer>

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
    QString lastEvent;
    QString userId;
    QString deviceId;
    std::vector<QString> needToken;

    mutable unsigned int txnCounter = 0;
    const qint64 txnBase = QDateTime::currentMSecsSinceEpoch();

    QString id() const { return userId + '/' + deviceId; }

    using job_queue_t = std::queue<QPointer<BaseJob>>;
    std::array<job_queue_t, 2> jobs; // 0 - foreground, 1 - background
    QTimer rateLimiter;
};

ConnectionData::ConnectionData(QUrl baseUrl)
    : d(std::make_unique<Private>(std::move(baseUrl)))
{
    // Each lambda invocation below takes no more than one job from the
    // queues (first foreground, then background) and resumes it; then
    // restarts the rate limiter timer with duration 0, effectively yielding
    // to the event loop and then resuming until both queues are empty.
    QObject::connect(&d->rateLimiter, &QTimer::timeout, [this] {
        // TODO: Consider moving out all job->sendRequest() invocations to
        // a dedicated thread
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
    qCDebug(MAIN) << job << "queued," << d->jobs.front().size() << "+"
                  << d->jobs.back().size() << "total jobs in" << d->id()
                  << "queues";
}

void ConnectionData::limitRate(std::chrono::milliseconds nextCallAfter)
{
    qCDebug(MAIN) << "Jobs for" << (d->userId + "/" + d->deviceId)
                  << "suspended for" << nextCallAfter.count() << "ms";
    d->rateLimiter.start(nextCallAfter);
}

QByteArray ConnectionData::accessToken() const { return d->accessToken; }

QUrl ConnectionData::baseUrl() const { return d->baseUrl; }

QNetworkAccessManager* ConnectionData::nam() const
{
    return NetworkAccessManager::instance();
}

void ConnectionData::setBaseUrl(QUrl baseUrl)
{
    d->baseUrl = std::move(baseUrl);
    qCDebug(MAIN) << "updated baseUrl to" << d->baseUrl;
}

void ConnectionData::setToken(QByteArray token)
{
    d->accessToken = std::move(token);
}

const QString& ConnectionData::deviceId() const { return d->deviceId; }

const QString& ConnectionData::userId() const { return d->userId; }

bool ConnectionData::needsToken(const QString& requestName) const
{
    return std::find(d->needToken.cbegin(), d->needToken.cend(), requestName)
           != d->needToken.cend();
}

void ConnectionData::setDeviceId(const QString& deviceId)
{
    d->deviceId = deviceId;
}

void ConnectionData::setUserId(const QString& userId) { d->userId = userId; }

void ConnectionData::setNeedsToken(const QString& requestName)
{
    d->needToken.push_back(requestName);
}

QString ConnectionData::lastEvent() const { return d->lastEvent; }

void ConnectionData::setLastEvent(QString identifier)
{
    d->lastEvent = std::move(identifier);
}

QByteArray ConnectionData::generateTxnId() const
{
    return d->deviceId.toLatin1() + QByteArray::number(d->txnBase)
           + QByteArray::number(++d->txnCounter);
}

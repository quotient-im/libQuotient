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

#include "basejob.h"

#include <array>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>
#include <QtCore/QTimer>

#include "../connectiondata.h"

using namespace QMatrixClient;

struct NetworkReplyDeleter : public QScopedPointerDeleteLater
{
    static inline void cleanup(QNetworkReply* reply)
    {
        if (reply && reply->isRunning())
            reply->abort();
        QScopedPointerDeleteLater::cleanup(reply);
    }
};

class BaseJob::Private
{
    public:
        Private(ConnectionData* c, JobHttpType t, QString endpoint,
                const QUrlQuery& q, const Data& data, bool nt)
            : connection(c), type(t), apiEndpoint(endpoint), requestQuery(q)
            , requestData(data), needsToken(nt)
            , reply(nullptr), status(NoError)
        { }
        
        inline void sendRequest();

        ConnectionData* connection;

        // Contents for the network request
        JobHttpType type;
        QString apiEndpoint;
        QUrlQuery requestQuery;
        Data requestData;
        bool needsToken;

        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        Status status;

        QTimer timer;
        QTimer retryTimer;

        size_t maxRetries = 3;
        size_t retriesTaken = 0;
};

inline QDebug operator<<(QDebug dbg, const BaseJob* j)
{
    return dbg << "Job" << j->objectName();
}

BaseJob::BaseJob(ConnectionData* connection, JobHttpType type, QString name,
                 QString endpoint, BaseJob::Query query, BaseJob::Data data,
                 bool needsToken)
    : d(new Private(connection, type, endpoint, query, data, needsToken))
{
    setObjectName(name);
    d->timer.setSingleShot(true);
    connect (&d->timer, &QTimer::timeout, this, &BaseJob::timeout);
    d->retryTimer.setSingleShot(true);
    connect (&d->retryTimer, &QTimer::timeout, this, &BaseJob::start);
    qDebug() << this << "created";
}

BaseJob::~BaseJob()
{
    stop();
    qDebug() << this << "destroyed";
}

ConnectionData* BaseJob::connection() const
{
    return d->connection;
}

const QUrlQuery&BaseJob::query() const
{
    return d->requestQuery;
}

void BaseJob::setRequestQuery(const QUrlQuery& query)
{
    d->requestQuery = query;
}

const BaseJob::Data&BaseJob::requestData() const
{
    return d->requestData;
}

void BaseJob::setRequestData(const BaseJob::Data& data)
{
    d->requestData = data;
}

void BaseJob::Private::sendRequest()
{
    QUrl url = connection->baseUrl();
    url.setPath( url.path() + "/" + apiEndpoint );
    if (needsToken)
        requestQuery.addQueryItem("access_token", connection->accessToken());
    url.setQuery(requestQuery);

    QNetworkRequest req {url};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    switch( type )
    {
        case JobHttpType::GetJob:
            reply.reset( connection->nam()->get(req) );
            break;
        case JobHttpType::PostJob:
            reply.reset( connection->nam()->post(req, requestData.serialize()) );
            break;
        case JobHttpType::PutJob:
            reply.reset( connection->nam()->put(req, requestData.serialize()) );
            break;
        case JobHttpType::DeleteJob:
            reply.reset( connection->nam()->deleteResource(req) );
            break;
    }
}

void BaseJob::start()
{
    emit aboutToStart();
    d->retryTimer.stop(); // In case we were counting down at the moment
    d->sendRequest();
    connect( d->reply.data(), &QNetworkReply::sslErrors, this, &BaseJob::sslErrors );
    connect( d->reply.data(), &QNetworkReply::finished, this, &BaseJob::gotReply );
    d->timer.start(getCurrentTimeout());
    emit started();
}

void BaseJob::gotReply()
{
    setStatus(checkReply(d->reply.data()));
    if (status().good())
        setStatus(parseReply(d->reply->readAll()));

    finishJob();
}

BaseJob::Status BaseJob::checkReply(QNetworkReply* reply) const
{
    if (reply->error() != QNetworkReply::NoError)
        qDebug() << this << "returned" << reply->error();
    switch( reply->error() )
    {
    case QNetworkReply::NoError:
        return NoError;

    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
        return { ContentAccessError, reply->errorString() };

    case QNetworkReply::ProtocolInvalidOperationError:
        return { IncorrectRequestError, reply->errorString() };

    case QNetworkReply::ContentNotFoundError:
        return { NotFoundError, reply->errorString() };

    default:
        return { NetworkError, reply->errorString() };
    }
}

BaseJob::Status BaseJob::parseReply(QByteArray data)
{
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(data, &error);
    if( error.error == QJsonParseError::NoError )
        return parseJson(json);
    else
        return { JsonParseError, error.errorString() };
}

BaseJob::Status BaseJob::parseJson(const QJsonDocument&)
{
    return Success;
}

void BaseJob::stop()
{
    d->timer.stop();
    if (!d->reply)
    {
        qWarning() << this << "stopped with empty network reply";
    }
    else if (d->reply->isRunning())
    {
        qWarning() << this << "stopped without ready network reply";
        d->reply->disconnect(this); // Ignore whatever comes from the reply
        d->reply->abort();
    }
}

void BaseJob::finishJob()
{
    stop();
    if ((error() == NetworkError || error() == TimeoutError)
            && d->retriesTaken < d->maxRetries)
    {
        const auto retryInterval = getNextRetryInterval();
        ++d->retriesTaken;
        qWarning() << this << "will take retry" << d->retriesTaken
                   << "in" << retryInterval/1000 << "s";
        d->retryTimer.start(retryInterval);
        emit retryScheduled(d->retriesTaken, retryInterval);
        return;
    }

    // Notify those interested in any completion of the job (including killing)
    emit finished(this);

    emit result(this);
    if (error())
        emit failure(this);
    else
        emit success(this);

    deleteLater();
}

BaseJob::duration_t BaseJob::getCurrentTimeout() const
{
    static const std::array<int, 4> timeouts = { 90, 90, 120, 120 };
    return timeouts[std::min(d->retriesTaken, timeouts.size() - 1)] * 1000;
}

BaseJob::duration_t BaseJob::getNextRetryInterval() const
{
    static const std::array<int, 3> intervals = { 5, 10, 30 };
    return intervals[std::min(d->retriesTaken, intervals.size() - 1)] * 1000;
}

BaseJob::duration_t BaseJob::millisToRetry() const
{
    return d->retryTimer.isActive() ? d->retryTimer.remainingTime() : 0;
}

size_t BaseJob::maxRetries() const
{
    return d->maxRetries;
}

void BaseJob::setMaxRetries(size_t newMaxRetries)
{
    d->maxRetries = newMaxRetries;
}

BaseJob::Status BaseJob::status() const
{
    return d->status;
}

int BaseJob::error() const
{
    return d->status.code;
}

QString BaseJob::errorString() const
{
    return d->status.message;
}

void BaseJob::setStatus(Status s)
{
    d->status = s;
    if (!s.good())
    {
        qWarning() << this << "status" << s.code << ":" << s.message;
    }
}

void BaseJob::setStatus(int code, QString message)
{
    setStatus({ code, message });
}

void BaseJob::abandon()
{
    deleteLater();
}

void BaseJob::timeout()
{
    setStatus( TimeoutError, "The job has timed out" );
    finishJob();
}

void BaseJob::sslErrors(const QList<QSslError>& errors)
{
    foreach (const QSslError &error, errors) {
        qWarning() << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}

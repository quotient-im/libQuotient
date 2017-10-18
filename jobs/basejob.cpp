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

#include "connectiondata.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QTimer>
#include <QtCore/QRegularExpression>

#include <array>

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
        // Using an idiom from clang-tidy:
        // http://clang.llvm.org/extra/clang-tidy/checks/modernize-pass-by-value.html
        Private(HttpVerb v, QString endpoint, QUrlQuery q, Data data, bool nt)
            : verb(v), apiEndpoint(std::move(endpoint))
            , requestQuery(std::move(q)), requestData(std::move(data))
            , needsToken(nt)
        { }
        
        void sendRequest();

        const ConnectionData* connection = nullptr;

        // Contents for the network request
        HttpVerb verb;
        QString apiEndpoint;
        QUrlQuery requestQuery;
        Data requestData;
        bool needsToken;

        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        Status status = NoError;

        QTimer timer;
        QTimer retryTimer;

        size_t maxRetries = 3;
        size_t retriesTaken = 0;

        LoggingCategory logCat = JOBS;
};

inline QDebug operator<<(QDebug dbg, const BaseJob* j)
{
    return dbg << j->objectName();
}

QDebug QMatrixClient::operator<<(QDebug dbg, const BaseJob::Status& s)
{
    QRegularExpression filter { "(access_token)=[-_A-Za-z0-9]+" };
    return dbg << s.code << ':'
               << QString(s.message).replace(filter, "\1=HIDDEN");
}

BaseJob::BaseJob(HttpVerb verb, const QString& name, const QString& endpoint,
                 const Query& query, const Data& data, bool needsToken)
    : d(new Private(verb, endpoint, query, data, needsToken))
{
    setObjectName(name);
    d->timer.setSingleShot(true);
    connect (&d->timer, &QTimer::timeout, this, &BaseJob::timeout);
    d->retryTimer.setSingleShot(true);
    connect (&d->retryTimer, &QTimer::timeout, this, &BaseJob::sendRequest);
}

BaseJob::~BaseJob()
{
    stop();
    qCDebug(d->logCat) << this << "destroyed";
}

const QString& BaseJob::apiEndpoint() const
{
    return d->apiEndpoint;
}

void BaseJob::setApiEndpoint(const QString& apiEndpoint)
{
    d->apiEndpoint = apiEndpoint;
}

const QUrlQuery& BaseJob::query() const
{
    return d->requestQuery;
}

void BaseJob::setRequestQuery(const QUrlQuery& query)
{
    d->requestQuery = query;
}

const BaseJob::Data& BaseJob::requestData() const
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
    QUrlQuery q = requestQuery;
    if (needsToken)
        q.addQueryItem("access_token", connection->accessToken());
    url.setQuery(q);

    QNetworkRequest req {url};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    switch( verb )
    {
        case HttpVerb::Get:
            reply.reset( connection->nam()->get(req) );
            break;
        case HttpVerb::Post:
            reply.reset( connection->nam()->post(req, requestData.serialize()) );
            break;
        case HttpVerb::Put:
            reply.reset( connection->nam()->put(req, requestData.serialize()) );
            break;
        case HttpVerb::Delete:
            reply.reset( connection->nam()->deleteResource(req) );
            break;
    }
}

void BaseJob::beforeStart(const ConnectionData* connData)
{
}

void BaseJob::start(const ConnectionData* connData)
{
    d->connection = connData;
    beforeStart(connData);
    sendRequest();
}

void BaseJob::sendRequest()
{
    emit aboutToStart();
    d->retryTimer.stop(); // In case we were counting down at the moment
    qCDebug(d->logCat) << this << "sending request to" << d->apiEndpoint;
    if (!d->requestQuery.isEmpty())
        qCDebug(d->logCat) << "  query:" << d->requestQuery.toString();
    d->sendRequest();
    connect( d->reply.data(), &QNetworkReply::sslErrors, this, &BaseJob::sslErrors );
    connect( d->reply.data(), &QNetworkReply::finished, this, &BaseJob::gotReply );
    if (d->reply->isRunning())
    {
        d->timer.start(getCurrentTimeout());
        qCDebug(d->logCat) << this << "request has been sent";
        emit started();
    }
    else
        qCWarning(d->logCat) << this << "request could not start";
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
        qCDebug(d->logCat) << this << "returned" << reply->error();
    switch( reply->error() )
    {
    case QNetworkReply::NoError:
        return NoError;

    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
        return { ContentAccessError, reply->errorString() };

    case QNetworkReply::ProtocolInvalidOperationError:
    case QNetworkReply::UnknownContentError:
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
    if (d->reply)
    {
        d->reply->disconnect(this); // Ignore whatever comes from the reply
        if (d->reply->isRunning())
        {
            qCWarning(d->logCat) << this << "stopped without ready network reply";
            d->reply->abort();
        }
    }
    else
        qCWarning(d->logCat) << this << "stopped with empty network reply";
}

void BaseJob::finishJob()
{
    stop();
    if ((error() == NetworkError || error() == TimeoutError)
            && d->retriesTaken < d->maxRetries)
    {
        // TODO: The whole retrying thing should be put to ConnectionManager
        // otherwise independently retrying jobs make a bit of notification
        // storm towards the UI.
        const auto retryInterval = getNextRetryInterval();
        ++d->retriesTaken;
        qCWarning(d->logCat) << this << "will take retry" << d->retriesTaken
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
        qCWarning(d->logCat) << this << "status" << s;
}

void BaseJob::setStatus(int code, QString message)
{
    setStatus({ code, message });
}

void BaseJob::abandon()
{
    this->disconnect();
    if (d->reply)
        d->reply->disconnect(this);
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
        qCWarning(d->logCat) << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}

void BaseJob::setLoggingCategory(LoggingCategory lcf)
{
    d->logCat = lcf;
}


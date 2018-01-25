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
//#include <QtCore/QStringBuilder>

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
        Private(HttpVerb v, QString endpoint, QUrlQuery q, Data&& data, bool nt)
            : verb(v), apiEndpoint(std::move(endpoint))
            , requestQuery(std::move(q)), requestData(std::move(data))
            , needsToken(nt)
        { }
        
        void sendRequest();
        const JobTimeoutConfig& getCurrentTimeoutConfig() const;

        const ConnectionData* connection = nullptr;

        // Contents for the network request
        HttpVerb verb;
        QString apiEndpoint;
        QHash<QByteArray, QByteArray> requestHeaders;
        QUrlQuery requestQuery;
        Data requestData;
        bool needsToken;

        // There's no use of QMimeType here because we don't want to match
        // content types against the known MIME type hierarchy; and at the same
        // type QMimeType is of little help with MIME type globs (`text/*` etc.)
        QByteArrayList expectedContentTypes;

        QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
        Status status = Pending;

        QTimer timer;
        QTimer retryTimer;

        QVector<JobTimeoutConfig> errorStrategy =
            { { 90, 5 }, { 90, 10 }, { 120, 30 } };
        int maxRetries = errorStrategy.size();
        int retriesTaken = 0;

        LoggingCategory logCat = JOBS;
};

inline QDebug operator<<(QDebug dbg, const BaseJob* j)
{
    return dbg << j->objectName();
}

QDebug QMatrixClient::operator<<(QDebug dbg, const BaseJob::Status& s)
{
    QRegularExpression filter { "(access_token)(=|: )[-_A-Za-z0-9]+" };
    return dbg << s.code << ':'
               << QString(s.message).replace(filter, "\\1 HIDDEN");
}

BaseJob::BaseJob(HttpVerb verb, const QString& name, const QString& endpoint, bool needsToken)
    : BaseJob(verb, name, endpoint, Query { }, Data { }, needsToken)
{ }

BaseJob::BaseJob(HttpVerb verb, const QString& name, const QString& endpoint,
                 const Query& query, Data&& data, bool needsToken)
    : d(new Private(verb, endpoint, query, std::move(data), needsToken))
{
    setObjectName(name);
    setExpectedContentTypes({ "application/json" });
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

const BaseJob::headers_t&BaseJob::requestHeaders() const
{
    return d->requestHeaders;
}

void BaseJob::setRequestHeader(const headers_t::key_type& headerName,
                               const headers_t::mapped_type& headerValue)
{
    d->requestHeaders[headerName] = headerValue;
}

void BaseJob::setRequestHeaders(const BaseJob::headers_t& headers)
{
    d->requestHeaders = headers;
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

void BaseJob::setRequestData(Data&& data)
{
    std::swap(d->requestData, data);
}

const QByteArrayList& BaseJob::expectedContentTypes() const
{
    return d->expectedContentTypes;
}

void BaseJob::addExpectedContentType(const QByteArray& contentType)
{
    d->expectedContentTypes << contentType;
}

void BaseJob::setExpectedContentTypes(const QByteArrayList& contentTypes)
{
    d->expectedContentTypes = contentTypes;
}

void BaseJob::Private::sendRequest()
{
    QUrl url = connection->baseUrl();
    QString path = url.path();
    if (!path.endsWith('/') && !apiEndpoint.startsWith('/'))
        path.push_back('/');

    url.setPath( path + apiEndpoint );
    url.setQuery(requestQuery);

    QNetworkRequest req {url};
    if (!requestHeaders.contains("Content-Type"))
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader(QByteArray("Authorization"),
                     QByteArray("Bearer ") + connection->accessToken());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    for (auto it = requestHeaders.cbegin(); it != requestHeaders.cend(); ++it)
        req.setRawHeader(it.key(), it.value());
    switch( verb )
    {
        case HttpVerb::Get:
            reply.reset( connection->nam()->get(req) );
            break;
        case HttpVerb::Post:
            reply.reset( connection->nam()->post(req, requestData.source()) );
            break;
        case HttpVerb::Put:
            reply.reset( connection->nam()->put(req, requestData.source()) );
            break;
        case HttpVerb::Delete:
            reply.reset( connection->nam()->deleteResource(req) );
            break;
    }
}

void BaseJob::beforeStart(const ConnectionData*)
{ }

void BaseJob::afterStart(const ConnectionData*, QNetworkReply*)
{ }

void BaseJob::beforeAbandon(QNetworkReply*)
{ }

void BaseJob::start(const ConnectionData* connData)
{
    d->connection = connData;
    beforeStart(connData);
    sendRequest();
    afterStart(connData, d->reply.data());
}

void BaseJob::sendRequest()
{
    emit aboutToStart();
    d->retryTimer.stop(); // In case we were counting down at the moment
    qCDebug(d->logCat) << this << "sending request to" << d->apiEndpoint;
    if (!d->requestQuery.isEmpty())
        qCDebug(d->logCat) << "  query:" << d->requestQuery.toString();
    d->sendRequest();
    connect( d->reply.data(), &QNetworkReply::finished, this, &BaseJob::gotReply );
    if (d->reply->isRunning())
    {
        connect( d->reply.data(), &QNetworkReply::uploadProgress,
                 this, &BaseJob::uploadProgress);
        connect( d->reply.data(), &QNetworkReply::downloadProgress,
                 this, &BaseJob::downloadProgress);
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
        setStatus(parseReply(d->reply.data()));

    finishJob();
}

bool checkContentType(const QByteArray& type, const QByteArrayList& patterns)
{
    if (patterns.isEmpty())
        return true;

    for (const auto& pattern: patterns)
    {
        if (pattern.startsWith('*') || type == pattern) // Fast lane
            return true;

        auto patternParts = pattern.split('/');
        Q_ASSERT_X(patternParts.size() <= 2, __FUNCTION__,
            "BaseJob: Expected content type should have up to two"
            " /-separated parts; violating pattern: " + pattern);

        if (type.split('/').front() == patternParts.front() &&
                patternParts.back() == "*")
            return true; // Exact match already went on fast lane
    }

    return false;
}

BaseJob::Status BaseJob::checkReply(QNetworkReply* reply) const
{
    qCDebug(d->logCat) << this << "returned"
        << (reply->error() == QNetworkReply::NoError ?
                "Success" : reply->errorString())
        << "from" << reply->url().toDisplayString();
    switch( reply->error() )
    {
        case QNetworkReply::NoError:
            if (checkContentType(reply->rawHeader("Content-Type"),
                                 d->expectedContentTypes))
                return NoError;
            else
                return { IncorrectResponseError,
                         "Incorrect content type of the response" };

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

BaseJob::Status BaseJob::parseReply(QNetworkReply* reply)
{
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(reply->readAll(), &error);
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

const JobTimeoutConfig& BaseJob::Private::getCurrentTimeoutConfig() const
{
    return errorStrategy[std::min(retriesTaken, errorStrategy.size() - 1)];
}

BaseJob::duration_t BaseJob::getCurrentTimeout() const
{
    return d->getCurrentTimeoutConfig().jobTimeout * 1000;
}

BaseJob::duration_t BaseJob::getNextRetryInterval() const
{
    return d->getCurrentTimeoutConfig().nextRetryInterval * 1000;
}

BaseJob::duration_t BaseJob::millisToRetry() const
{
    return d->retryTimer.isActive() ? d->retryTimer.remainingTime() : 0;
}

int BaseJob::maxRetries() const
{
    return d->maxRetries;
}

void BaseJob::setMaxRetries(int newMaxRetries)
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
    beforeAbandon(d->reply.data());
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

void BaseJob::setLoggingCategory(LoggingCategory lcf)
{
    d->logCat = lcf;
}

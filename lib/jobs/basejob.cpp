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
#include "util.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QJsonObject>

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
        Private(HttpVerb v, QString endpoint, const QUrlQuery& q,
                Data&& data, bool nt)
            : verb(v), apiEndpoint(std::move(endpoint)), requestQuery(q)
            , requestData(std::move(data)), needsToken(nt)
        { }
        
        void sendRequest(bool inBackground);
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
        QByteArray rawResponse;
        QUrl errorUrl; //< May contain a URL to help with some errors

        QTimer timer;
        QTimer retryTimer;

        QVector<JobTimeoutConfig> errorStrategy =
            { { 90, 5 }, { 90, 10 }, { 120, 30 } };
        int maxRetries = errorStrategy.size();
        int retriesTaken = 0;

        LoggingCategory logCat = JOBS;
};

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
}

BaseJob::~BaseJob()
{
    stop();
    qCDebug(d->logCat) << this << "destroyed";
}

QUrl BaseJob::requestUrl() const
{
    return d->reply ? d->reply->request().url() : QUrl();
}

bool BaseJob::isBackground() const
{
    return d->reply && d->reply->request().attribute(
                        QNetworkRequest::BackgroundRequestAttribute).toBool();
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

QUrl BaseJob::makeRequestUrl(QUrl baseUrl,
                             const QString& path, const QUrlQuery& query)
{
    auto pathBase = baseUrl.path();
    if (!pathBase.endsWith('/') && !path.startsWith('/'))
        pathBase.push_back('/');

    baseUrl.setPath(pathBase + path, QUrl::TolerantMode);
    baseUrl.setQuery(query);
    return baseUrl;
}

void BaseJob::Private::sendRequest(bool inBackground)
{
    QNetworkRequest req
        { makeRequestUrl(connection->baseUrl(), apiEndpoint, requestQuery) };
    if (!requestHeaders.contains("Content-Type"))
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QByteArray("Bearer ") + connection->accessToken());
    req.setAttribute(QNetworkRequest::BackgroundRequestAttribute, inBackground);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    // some sources claim that there are issues with QT 5.8
    req.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);
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

void BaseJob::start(const ConnectionData* connData, bool inBackground)
{
    d->connection = connData;
    d->retryTimer.setSingleShot(true);
    connect (&d->retryTimer, &QTimer::timeout,
             this, [this,inBackground] { sendRequest(inBackground); });

    beforeStart(connData);
    if (status().good())
        sendRequest(inBackground);
    if (status().good())
        afterStart(connData, d->reply.data());
    if (!status().good())
        QTimer::singleShot(0, this, &BaseJob::finishJob);
}

void BaseJob::sendRequest(bool inBackground)
{
    emit aboutToStart();
    d->retryTimer.stop(); // In case we were counting down at the moment
    qCDebug(d->logCat) << this << "sending request to" << d->apiEndpoint;
    if (!d->requestQuery.isEmpty())
        qCDebug(d->logCat) << "  query:" << d->requestQuery.toString();
    d->sendRequest(inBackground);
    connect( d->reply.data(), &QNetworkReply::finished, this, &BaseJob::gotReply );
    if (d->reply->isRunning())
    {
        connect( d->reply.data(), &QNetworkReply::metaDataChanged,
                 this, &BaseJob::checkReply);
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

void BaseJob::checkReply()
{
    setStatus(doCheckReply(d->reply.data()));
}

void BaseJob::gotReply()
{
    checkReply();
    if (status().good())
        setStatus(parseReply(d->reply.data()));
    else {
        d->rawResponse = d->reply->readAll();
        const auto jsonBody =
            d->reply->rawHeader("Content-Type") == "application/json";
        qCDebug(d->logCat).noquote()
            << "Error body (truncated if long):" << d->rawResponse.left(500);
        if (jsonBody)
            setStatus(
                parseError(d->reply.data(),
                           QJsonDocument::fromJson(d->rawResponse).object()));
    }

    if (status().code != TooManyRequestsError)
        finishJob();
    else {
        stop();
        emit retryScheduled(d->retriesTaken, d->retryTimer.interval());
    }
}

bool checkContentType(const QByteArray& type, const QByteArrayList& patterns)
{
    if (patterns.isEmpty())
        return true;

    // ignore possible appendixes of the content type
    const auto ctype = type.split(';').front();

    for (const auto& pattern: patterns)
    {
        if (pattern.startsWith('*') || ctype == pattern) // Fast lane
            return true;

        auto patternParts = pattern.split('/');
        Q_ASSERT_X(patternParts.size() <= 2, __FUNCTION__,
            "BaseJob: Expected content type should have up to two"
            " /-separated parts; violating pattern: " + pattern);

        if (ctype.split('/').front() == patternParts.front() &&
                patternParts.back() == "*")
            return true; // Exact match already went on fast lane
    }

    return false;
}

BaseJob::Status BaseJob::doCheckReply(QNetworkReply* reply) const
{
    // QNetworkReply error codes seem to be flawed when it comes to HTTP;
    // see, e.g., https://github.com/QMatrixClient/libqmatrixclient/issues/200
    // so check genuine HTTP codes. The below processing is based on
    // https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
    const auto httpCodeHeader =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (!httpCodeHeader.isValid())
    {
        qCWarning(d->logCat) << this << "didn't get valid HTTP headers";
        return { NetworkError, reply->errorString() };
    }

    const QString replyState = reply->isRunning() ?
                QStringLiteral("(tentative)") : QStringLiteral("(final)");
    const auto urlString = '|' + d->reply->url().toDisplayString();
    const auto httpCode = httpCodeHeader.toInt();
    const auto reason =
        reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    if (httpCode / 100 == 2) // 2xx
    {
        qCDebug(d->logCat).noquote().nospace() << this << urlString;
        qCDebug(d->logCat).noquote() << "  " << httpCode << reason << replyState;
        if (!checkContentType(reply->rawHeader("Content-Type"),
                             d->expectedContentTypes))
            return { UnexpectedResponseTypeWarning,
                     "Unexpected content type of the response" };
        return NoError;
    }

    qCWarning(d->logCat).noquote().nospace() << this << urlString;
    qCWarning(d->logCat).noquote() << "  " << httpCode << reason << replyState;
    return { [httpCode]() -> StatusCode {
            if (httpCode / 10 == 41)
                return httpCode == 410 ? IncorrectRequestError : NotFoundError;
            switch (httpCode)
            {
                case 401: case 403: case 407:
                    return ContentAccessError;
                case 404:
                    return NotFoundError;
                case 400: case 405: case 406: case 426: case 428:
                case 505:
                case 494: // Unofficial nginx "Request header too large"
                case 497: // Unofficial nginx "HTTP request sent to HTTPS port"
                    return IncorrectRequestError;
                case 429:
                    return TooManyRequestsError;
                case 501: case 510:
                    return RequestNotImplementedError;
                case 511:
                    return NetworkAuthRequiredError;
                default:
                    return NetworkError;
            }
        }(), reply->errorString() };
}

BaseJob::Status BaseJob::parseReply(QNetworkReply* reply)
{
    d->rawResponse = reply->readAll();
    QJsonParseError error { 0, QJsonParseError::MissingObject };
    const auto& json = QJsonDocument::fromJson(d->rawResponse, &error);
    if( error.error == QJsonParseError::NoError )
        return parseJson(json);

    return { IncorrectResponseError, error.errorString() };
}

BaseJob::Status BaseJob::parseJson(const QJsonDocument&)
{
    return Success;
}

BaseJob::Status BaseJob::parseError(QNetworkReply* reply,
                                    const QJsonObject& errorJson)
{
    const auto errCode = errorJson.value("errcode"_ls).toString();
    if (error() == TooManyRequestsError || errCode == "M_LIMIT_EXCEEDED") {
        QString msg = tr("Too many requests");
        auto retryInterval = errorJson.value("retry_after_ms"_ls).toInt(-1);
        if (retryInterval != -1)
            msg += tr(", next retry advised after %1 ms").arg(retryInterval);
        else // We still have to figure some reasonable interval
            retryInterval = getNextRetryInterval();

        qCWarning(d->logCat) << this << "will retry in" << retryInterval << "ms";
        d->retryTimer.start(retryInterval);

        return { TooManyRequestsError, msg };
    }
    if (errCode == "M_CONSENT_NOT_GIVEN") {
        d->errorUrl = errorJson.value("consent_uri"_ls).toString();
        return { UserConsentRequiredError };
    }
    if (errCode == "M_UNSUPPORTED_ROOM_VERSION"
        || errCode == "M_INCOMPATIBLE_ROOM_VERSION")
        return { UnsupportedRoomVersionError,
                 errorJson.contains("room_version"_ls)
                     ? tr("Requested room version: %1")
                           .arg(errorJson.value("room_version"_ls).toString())
                     : errorJson.value("error"_ls).toString() };

    // Not localisable on the client side
    if (errorJson.contains("error"_ls))
        d->status.message = errorJson.value("error"_ls).toString();

    return d->status;
}

void BaseJob::stop()
{
    // This method is used to semi-finalise the job before retrying; so
    // stop the timeout timer but keep the retry timer running.
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
        const auto retryInterval =
                error() == TimeoutError ? 0 : getNextRetryInterval();
        ++d->retriesTaken;
        qCWarning(d->logCat).nospace() << this << ": retry #" << d->retriesTaken
                   << " in " << retryInterval/1000 << " s";
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

QByteArray BaseJob::rawData(int bytesAtMost) const
{
    return bytesAtMost > 0 && d->rawResponse.size() > bytesAtMost
            ? d->rawResponse.left(bytesAtMost) : d->rawResponse;
}

QString BaseJob::rawDataSample(int bytesAtMost) const
{
    auto data = rawData(bytesAtMost);
    Q_ASSERT(data.size() <= d->rawResponse.size());
    return data.size() == d->rawResponse.size()
            ? data : data + tr("...(truncated, %Ln bytes in total)",
                               "Comes after trimmed raw network response",
                               d->rawResponse.size());

}

QString BaseJob::statusCaption() const
{
    switch (d->status.code)
    {
        case Success:
            return tr("Success");
        case Pending:
            return tr("Request still pending response");
        case UnexpectedResponseTypeWarning:
            return tr("Warning: Unexpected response type");
        case Abandoned:
            return tr("Request was abandoned");
        case NetworkError:
            return tr("Network problems");
        case TimeoutError:
            return tr("Request timed out");
        case ContentAccessError:
            return tr("Access error");
        case NotFoundError:
            return tr("Not found");
        case IncorrectRequestError:
            return tr("Invalid request");
        case IncorrectResponseError:
            return tr("Response could not be parsed");
        case TooManyRequestsError:
            return tr("Too many requests");
        case RequestNotImplementedError:
            return tr("Function not implemented by the server");
        case NetworkAuthRequiredError:
            return tr("Network authentication required");
        case UserConsentRequiredError:
            return tr("User consent required");
        case UnsupportedRoomVersionError:
            return tr("The server does not support the needed room version");
        default:
            return tr("Request failed");
    }
}

int BaseJob::error() const
{
    return d->status.code;
}

QString BaseJob::errorString() const
{
    return d->status.message;
}

QUrl BaseJob::errorUrl() const
{
    return d->errorUrl;
}

void BaseJob::setStatus(Status s)
{
    // The crash that led to this code has been reported in
    // https://github.com/QMatrixClient/Quaternion/issues/566 - basically,
    // when cleaning up childrent of a deleted Connection, there's a chance
    // of pending jobs being abandoned, calling setStatus(Abandoned).
    // There's nothing wrong with this; however, the safety check for
    // cleartext access tokens below uses d->connection - which is a dangling
    // pointer.
    // To alleviate that, a stricter condition is applied, that for Abandoned
    // and to-be-Abandoned jobs the status message will be disregarded entirely.
    // For 0.6 we might rectify the situation by making d->connection
    // a QPointer<> (and derive ConnectionData from QObject, respectively).
    if (d->status.code == Abandoned || s.code == Abandoned)
        s.message.clear();

    if (d->status == s)
        return;

    if (!s.message.isEmpty()
            && d->connection && !d->connection->accessToken().isEmpty())
        s.message.replace(d->connection->accessToken(), "(REDACTED)");
    if (!s.good())
        qCWarning(d->logCat) << this << "status" << s;
    d->status = std::move(s);
    emit statusChanged(d->status);
}

void BaseJob::setStatus(int code, QString message)
{
    setStatus({ code, std::move(message) });
}

void BaseJob::abandon()
{
    beforeAbandon(d->reply ? d->reply.data() : nullptr);
    setStatus(Abandoned);
    if (d->reply)
        d->reply->disconnect(this);
    emit finished(this);

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

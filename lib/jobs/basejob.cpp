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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "basejob.h"

#include "connectiondata.h"
#include "util.h"

#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtCore/QStringBuilder>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <array>

using namespace Quotient;
using std::chrono::seconds, std::chrono::milliseconds;
using namespace std::chrono_literals;

struct NetworkReplyDeleter : public QScopedPointerDeleteLater {
    static inline void cleanup(QNetworkReply* reply)
    {
        if (reply && reply->isRunning())
            reply->abort();
        QScopedPointerDeleteLater::cleanup(reply);
    }
};

template <typename... Ts>
constexpr auto make_array(Ts&&... items)
{
    return std::array<std::common_type_t<Ts...>, sizeof...(Ts)>({items...});
}

class BaseJob::Private {
public:
    struct JobTimeoutConfig {
        seconds jobTimeout;
        seconds nextRetryInterval;
    };

    // Using an idiom from clang-tidy:
    // http://clang.llvm.org/extra/clang-tidy/checks/modernize-pass-by-value.html
    Private(HttpVerb v, QString endpoint, const QUrlQuery& q, Data&& data,
            bool nt)
        : verb(v)
        , apiEndpoint(std::move(endpoint))
        , requestQuery(q)
        , requestData(std::move(data))
        , needsToken(nt)
    {
        timer.setSingleShot(true);
        retryTimer.setSingleShot(true);
    }

    void sendRequest();

    ConnectionData* connection = nullptr;

    // Contents for the network request
    HttpVerb verb;
    QString apiEndpoint;
    QHash<QByteArray, QByteArray> requestHeaders;
    QUrlQuery requestQuery;
    Data requestData;
    bool needsToken;

    bool inBackground = false;

    // There's no use of QMimeType here because we don't want to match
    // content types against the known MIME type hierarchy; and at the same
    // type QMimeType is of little help with MIME type globs (`text/*` etc.)
    QByteArrayList expectedContentTypes { "application/json" };

    QScopedPointer<QNetworkReply, NetworkReplyDeleter> reply;
    Status status = Unprepared;
    QByteArray rawResponse;
    QUrl errorUrl; //< May contain a URL to help with some errors

    LoggingCategory logCat = JOBS;

    QTimer timer;
    QTimer retryTimer;

    static constexpr std::array<const JobTimeoutConfig, 3> errorStrategy {
        { { 90s, 5s }, { 90s, 10s }, { 120s, 30s } }
    };
    int maxRetries = int(errorStrategy.size());
    int retriesTaken = 0;

    [[nodiscard]] const JobTimeoutConfig& getCurrentTimeoutConfig() const
    {
        return errorStrategy[std::min(size_t(retriesTaken),
                                      errorStrategy.size() - 1)];
    }

    [[nodiscard]] QString dumpRequest() const
    {
        // FIXME: use std::array {} when Apple stdlib gets deduction guides for it
        static const auto verbs =
            make_array(QStringLiteral("GET"), QStringLiteral("PUT"),
                       QStringLiteral("POST"), QStringLiteral("DELETE"));
        const auto verbWord = verbs.at(size_t(verb));
        return verbWord % ' '
               % (reply ? reply->url().toString(QUrl::RemoveQuery)
                        : makeRequestUrl(connection->baseUrl(), apiEndpoint)
                              .toString());
    }
};

BaseJob::BaseJob(HttpVerb verb, const QString& name, const QString& endpoint,
                 bool needsToken)
    : BaseJob(verb, name, endpoint, Query {}, Data {}, needsToken)
{}

BaseJob::BaseJob(HttpVerb verb, const QString& name, const QString& endpoint,
                 const Query& query, Data&& data, bool needsToken)
    : d(new Private(verb, endpoint, query, std::move(data), needsToken))
{
    setObjectName(name);
    connect(&d->timer, &QTimer::timeout, this, &BaseJob::timeout);
    connect(&d->retryTimer, &QTimer::timeout, this, [this] {
        setStatus(Pending);
        sendRequest();
    });
}

BaseJob::~BaseJob()
{
    stop();
    qCDebug(d->logCat) << this << "destroyed";
}

QUrl BaseJob::requestUrl() const
{
    return d->reply ? d->reply->url() : QUrl();
}

bool BaseJob::isBackground() const
{
    return d->inBackground;
}

const QString& BaseJob::apiEndpoint() const { return d->apiEndpoint; }

void BaseJob::setApiEndpoint(const QString& apiEndpoint)
{
    d->apiEndpoint = apiEndpoint;
}

const BaseJob::headers_t& BaseJob::requestHeaders() const
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

const QUrlQuery& BaseJob::query() const { return d->requestQuery; }

void BaseJob::setRequestQuery(const QUrlQuery& query)
{
    d->requestQuery = query;
}

const BaseJob::Data& BaseJob::requestData() const { return d->requestData; }

void BaseJob::setRequestData(Data&& data) { std::swap(d->requestData, data); }

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

QUrl BaseJob::makeRequestUrl(QUrl baseUrl, const QString& path,
                             const QUrlQuery& query)
{
    auto pathBase = baseUrl.path();
    if (!pathBase.endsWith('/') && !path.startsWith('/'))
        pathBase.push_back('/');

    baseUrl.setPath(pathBase + path, QUrl::TolerantMode);
    baseUrl.setQuery(query);
    return baseUrl;
}

void BaseJob::Private::sendRequest()
{
    QNetworkRequest req { makeRequestUrl(connection->baseUrl(), apiEndpoint,
                                         requestQuery) };
    if (!requestHeaders.contains("Content-Type"))
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization",
                     QByteArray("Bearer ") + connection->accessToken());
    req.setAttribute(QNetworkRequest::BackgroundRequestAttribute, inBackground);
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    req.setAttribute(QNetworkRequest::HTTP2AllowedAttribute, true);
    for (auto it = requestHeaders.cbegin(); it != requestHeaders.cend(); ++it)
        req.setRawHeader(it.key(), it.value());

    switch (verb) {
    case HttpVerb::Get:
        reply.reset(connection->nam()->get(req));
        break;
    case HttpVerb::Post:
        reply.reset(connection->nam()->post(req, requestData.source()));
        break;
    case HttpVerb::Put:
        reply.reset(connection->nam()->put(req, requestData.source()));
        break;
    case HttpVerb::Delete:
        reply.reset(connection->nam()->deleteResource(req));
        break;
    }
}

void BaseJob::doPrepare() {}

void BaseJob::onSentRequest(QNetworkReply*) {}

void BaseJob::beforeAbandon(QNetworkReply*) {}

void BaseJob::prepare(ConnectionData* connData, bool inBackground)
{
    d->inBackground = inBackground;
    d->connection = connData;
    doPrepare();
    if (status().code != Unprepared && status().code != Pending)
        QTimer::singleShot(0, this, &BaseJob::finishJob);
    setStatus(Pending);
}

void BaseJob::sendRequest()
{
    if (status().code == Abandoned)
        return;
    Q_ASSERT(d->connection && status().code == Pending);
    qCDebug(d->logCat).noquote() << "Making" << d->dumpRequest();
    emit aboutToSendRequest();
    d->sendRequest();
    Q_ASSERT(d->reply);
    connect(d->reply.data(), &QNetworkReply::finished, this, &BaseJob::gotReply);
    if (d->reply->isRunning()) {
        connect(d->reply.data(), &QNetworkReply::metaDataChanged, this,
                &BaseJob::checkReply);
        connect(d->reply.data(), &QNetworkReply::uploadProgress, this,
                &BaseJob::uploadProgress);
        connect(d->reply.data(), &QNetworkReply::downloadProgress, this,
                &BaseJob::downloadProgress);
        d->timer.start(getCurrentTimeout());
        qCInfo(d->logCat).noquote() << "Sent" << d->dumpRequest();
        onSentRequest(d->reply.data());
        emit sentRequest();
    } else
        qCWarning(d->logCat).noquote()
            << "Request could not start:" << d->dumpRequest();
}

void BaseJob::checkReply() { setStatus(doCheckReply(d->reply.data())); }

void BaseJob::gotReply()
{
    checkReply();
    if (status().good())
        setStatus(parseReply(d->reply.data()));
    else {
        d->rawResponse = d->reply->readAll();
        const auto jsonBody = d->reply->rawHeader("Content-Type")
                              == "application/json";
        qCDebug(d->logCat).noquote()
            << "Error body (truncated if long):" << d->rawResponse.left(500);
        if (jsonBody)
            setStatus(
                parseError(d->reply.data(),
                           QJsonDocument::fromJson(d->rawResponse).object()));
    }
    finishJob();
}

bool checkContentType(const QByteArray& type, const QByteArrayList& patterns)
{
    if (patterns.isEmpty())
        return true;

    // ignore possible appendixes of the content type
    const auto ctype = type.split(';').front();

    for (const auto& pattern : patterns) {
        if (pattern.startsWith('*') || ctype == pattern) // Fast lane
            return true;

        auto patternParts = pattern.split('/');
        Q_ASSERT_X(patternParts.size() <= 2, __FUNCTION__,
                   "BaseJob: Expected content type should have up to two"
                   " /-separated parts; violating pattern: "
                       + pattern);

        if (ctype.split('/').front() == patternParts.front()
            && patternParts.back() == "*")
            return true; // Exact match already went on fast lane
    }

    return false;
}

BaseJob::Status BaseJob::Status::fromHttpCode(int httpCode, QString msg)
{
    // clang-format off
    return { [httpCode]() -> StatusCode {
            if (httpCode / 10 == 41) // 41x errors
                return httpCode == 410 ? IncorrectRequestError : NotFoundError;
            switch (httpCode) {
            case 401: case 403: case 407:
                return ContentAccessError;
            case 404:
                return NotFoundError;
            case 400: case 405: case 406: case 426: case 428: case 505:
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
        }(), std::move(msg) };
    // clang-format on
}

QDebug BaseJob::Status::dumpToLog(QDebug dbg) const
{
    QDebugStateSaver _s(dbg);
    dbg.noquote().nospace();
    if (auto* const k = QMetaEnum::fromType<StatusCode>().valueToKey(code)) {
        const QByteArray b = k;
        dbg << b.mid(b.lastIndexOf(':'));
    } else
        dbg << code;
    return dbg << ": " << message;

}

BaseJob::Status BaseJob::doCheckReply(QNetworkReply* reply) const
{
    // QNetworkReply error codes seem to be flawed when it comes to HTTP;
    // see, e.g., https://github.com/quotient-im/libQuotient/issues/200
    // so check genuine HTTP codes. The below processing is based on
    // https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
    const auto httpCodeHeader =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (!httpCodeHeader.isValid()) {
        qCWarning(d->logCat).noquote()
            << "No valid HTTP headers from" << d->dumpRequest();
        return { NetworkError, reply->errorString() };
    }

    const auto httpCode = httpCodeHeader.toInt();
    if (httpCode / 100 == 2) // 2xx
    {
        if (reply->isFinished())
            qCInfo(d->logCat).noquote() << httpCode << "<-" << d->dumpRequest();
        if (!checkContentType(reply->rawHeader("Content-Type"),
                              d->expectedContentTypes))
            return { UnexpectedResponseTypeWarning,
                     "Unexpected content type of the response" };
        return NoError;
    }
    if (reply->isFinished())
        qCWarning(d->logCat).noquote() << httpCode << "<-" << d->dumpRequest();

    auto message = reply->errorString();
    if (message.isEmpty())
        message = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute)
                      .toString();

    return Status::fromHttpCode(httpCode, message);
}

BaseJob::Status BaseJob::parseReply(QNetworkReply* reply)
{
    d->rawResponse = reply->readAll();
    QJsonParseError error { 0, QJsonParseError::MissingObject };
    const auto& json = QJsonDocument::fromJson(d->rawResponse, &error);
    if (error.error == QJsonParseError::NoError)
        return parseJson(json);

    return { IncorrectResponseError, error.errorString() };
}

BaseJob::Status BaseJob::parseJson(const QJsonDocument&) { return Success; }

BaseJob::Status BaseJob::parseError(QNetworkReply*  /*reply*/,
                                    const QJsonObject& errorJson)
{
    const auto errCode = errorJson.value("errcode"_ls).toString();
    if (error() == TooManyRequestsError || errCode == "M_LIMIT_EXCEEDED") {
        QString msg = tr("Too many requests");
        int64_t retryAfterMs = errorJson.value("retry_after_ms"_ls).toInt(-1);
        if (retryAfterMs >= 0)
            msg += tr(", next retry advised after %1 ms").arg(retryAfterMs);
        else // We still have to figure some reasonable interval
            retryAfterMs = getNextRetryMs();

        d->connection->limitRate(milliseconds(retryAfterMs));

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
    if (errCode == "M_CANNOT_LEAVE_SERVER_NOTICE_ROOM")
        return { CannotLeaveRoom,
                 tr("It's not allowed to leave a server notices room") };
    if (errCode == "M_USER_DEACTIVATED")
        return { UserDeactivated };

    // Not localisable on the client side
    if (errorJson.contains("error"_ls))
        d->status.message = errorJson.value("error"_ls).toString();

    return d->status;
}

void BaseJob::stop()
{
    // This method is (also) used to semi-finalise the job before retrying; so
    // stop the timeout timer but keep the retry timer running.
    d->timer.stop();
    if (d->reply) {
        d->reply->disconnect(this); // Ignore whatever comes from the reply
        if (d->reply->isRunning()) {
            qCWarning(d->logCat)
                << this << "stopped without ready network reply";
            d->reply->abort(); // Keep the reply object in case clients need it
        }
    } else
        qCWarning(d->logCat) << this << "stopped with empty network reply";
}

void BaseJob::finishJob()
{
    stop();
    if (error() == TooManyRequests) {
        emit rateLimited();
        setStatus(Pending);
        d->connection->submit(this);
        return;
    }
    if ((error() == NetworkError || error() == Timeout)
        && d->retriesTaken < d->maxRetries) {
        // TODO: The whole retrying thing should be put to Connection(Manager)
        // otherwise independently retrying jobs make a bit of notification
        // storm towards the UI.
        const seconds retryIn = error() == Timeout ? 0s : getNextRetryInterval();
        ++d->retriesTaken;
        qCWarning(d->logCat).nospace() << this << ": retry #" << d->retriesTaken
                                       << " in " << retryIn.count() << " s";
        d->retryTimer.start(retryIn);
        emit retryScheduled(d->retriesTaken, milliseconds(retryIn).count());
        return;
    }

    // Notify those interested in any completion of the job including abandon()
    emit finished(this);

    emit result(this); // abandon() doesn't emit this
    if (error())
        emit failure(this);
    else
        emit success(this);

    deleteLater();
}

seconds BaseJob::getCurrentTimeout() const
{
    return d->getCurrentTimeoutConfig().jobTimeout;
}

BaseJob::duration_ms_t BaseJob::getCurrentTimeoutMs() const
{
    return milliseconds(getCurrentTimeout()).count();
}

seconds BaseJob::getNextRetryInterval() const
{
    return d->getCurrentTimeoutConfig().nextRetryInterval;
}

BaseJob::duration_ms_t BaseJob::getNextRetryMs() const
{
    return milliseconds(getNextRetryInterval()).count();
}

milliseconds BaseJob::timeToRetry() const
{
    return d->retryTimer.isActive() ? d->retryTimer.remainingTimeAsDuration()
                                    : 0s;
}

BaseJob::duration_ms_t BaseJob::millisToRetry() const
{
    return timeToRetry().count();
}

int BaseJob::maxRetries() const { return d->maxRetries; }

void BaseJob::setMaxRetries(int newMaxRetries)
{
    d->maxRetries = newMaxRetries;
}

BaseJob::Status BaseJob::status() const { return d->status; }

QByteArray BaseJob::rawData(int bytesAtMost) const
{
    return bytesAtMost > 0 && d->rawResponse.size() > bytesAtMost
               ? d->rawResponse.left(bytesAtMost)
               : d->rawResponse;
}

QString BaseJob::rawDataSample(int bytesAtMost) const
{
    auto data = rawData(bytesAtMost);
    Q_ASSERT(data.size() <= d->rawResponse.size());
    return data.size() == d->rawResponse.size()
               ? data
               : data
                     + tr("...(truncated, %Ln bytes in total)",
                          "Comes after trimmed raw network response",
                          d->rawResponse.size());
}

QString BaseJob::statusCaption() const
{
    switch (d->status.code) {
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

int BaseJob::error() const { return d->status.code; }

QString BaseJob::errorString() const { return d->status.message; }

QUrl BaseJob::errorUrl() const { return d->errorUrl; }

void BaseJob::setStatus(Status s)
{
    // The crash that led to this code has been reported in
    // https://github.com/quotient-im/Quaternion/issues/566 - basically,
    // when cleaning up children of a deleted Connection, there's a chance
    // of pending jobs being abandoned, calling setStatus(Abandoned).
    // There's nothing wrong with this; however, the safety check for
    // cleartext access tokens below uses d->connection - which is a dangling
    // pointer.
    // To alleviate that, a stricter condition is applied, that for Abandoned
    // and to-be-Abandoned jobs the status message will be disregarded entirely.
    // We could rectify the situation by making d->connection a QPointer<>
    // (and deriving ConnectionData from QObject, respectively) but it's
    // a too edge case for the hassle.
    if (d->status == s)
        return;

    if (d->status.code == Abandoned || s.code == Abandoned)
        s.message.clear();

    if (!s.message.isEmpty() && d->connection
        && !d->connection->accessToken().isEmpty())
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
    setStatus(TimeoutError, "The job has timed out");
    finishJob();
}

void BaseJob::setLoggingCategory(LoggingCategory lcf) { d->logCat = lcf; }

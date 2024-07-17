// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "basejob.h"

#include "../logging_categories_p.h"

#include "../connectiondata.h"
#include "../networkaccessmanager.h"

#include <QtCore/QMetaEnum>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

using namespace Quotient;
using std::chrono::seconds, std::chrono::milliseconds;
using namespace std::chrono_literals;

BaseJob::StatusCode BaseJob::Status::fromHttpCode(int httpCode)
{
    // Based on https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
    if (httpCode / 10 == 41) // 41x errors
        return httpCode == 410 ? IncorrectRequest : NotFound;
    switch (httpCode) {
    case 401:
        return Unauthorised;
        // clang-format off
    case 403: case 407: // clang-format on
        return ContentAccessError;
    case 404:
        return NotFound;
        // clang-format off
    case 400: case 405: case 406: case 426: case 428: case 505: // clang-format on
    case 494: // Unofficial nginx "Request header too large"
    case 497: // Unofficial nginx "HTTP request sent to HTTPS port"
        return IncorrectRequest;
    case 429:
        return TooManyRequests;
    case 501:
    case 510:
        return RequestNotImplemented;
    case 511:
        return NetworkAuthRequired;
    default:
        return NetworkError;
    }
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

class Q_DECL_HIDDEN BaseJob::Private {
public:
    struct JobTimeoutConfig {
        seconds jobTimeout;
        seconds nextRetryInterval;
    };

    // Using an idiom from clang-tidy:
    // http://clang.llvm.org/extra/clang-tidy/checks/modernize-pass-by-value.html
    Private(HttpVerb v, QByteArray endpoint, const QUrlQuery& q,
            RequestData&& data, bool nt)
        : verb(v)
        , apiEndpoint(std::move(endpoint))
        , requestQuery(q)
        , requestData(std::move(data))
        , needsToken(nt)
    {
        timer.setSingleShot(true);
        retryTimer.setSingleShot(true);
    }

    ~Private()
    {
        if (reply) {
            if (reply->isRunning()) {
                reply->abort();
            }
            delete reply;
        }
    }

    QNetworkRequest prepareRequest() const;
    void sendRequest(const QNetworkRequest& req);

    /*! \brief Parse the response byte array into JSON
     *
     * This calls QJsonDocument::fromJson() on rawResponse, converts
     * the QJsonParseError result to BaseJob::Status and stores the resulting
     * JSON in jsonResponse.
     */
    Status parseJson();

    ConnectionData* connection = nullptr;

    // Contents for the network request
    HttpVerb verb;
    QByteArray apiEndpoint;
    QHash<QByteArray, QByteArray> requestHeaders;
    QUrlQuery requestQuery;
    RequestData requestData;
    bool needsToken;

    bool inBackground = false;

    // There's no use of QMimeType here because we don't want to match
    // content types against the known MIME type hierarchy; and at the same
    // type QMimeType is of little help with MIME type globs (`text/*` etc.)
    QByteArrayList expectedContentTypes { "application/json" };

    QByteArrayList expectedKeys;

    // When the QNetworkAccessManager is destroyed it destroys all pending replies.
    // Using QPointer allows us to know when that happend.
    QPointer<QNetworkReply> reply;

    QPromise<void> promise{};

    Status status = Unprepared;
    QByteArray rawResponse;
    /// Contains a null document in case of non-JSON body (for a successful
    /// or unsuccessful response); a document with QJsonObject or QJsonArray
    /// in case of a successful response with JSON payload, as per the API
    /// definition (including an empty JSON object - QJsonObject{});
    /// and QJsonObject in case of an API error.
    QJsonDocument jsonResponse;
    QUrl errorUrl; //!< May contain a URL to help with some errors

    QMessageLogger::CategoryFunction logCat = &JOBS;

    QTimer timer;
    QTimer retryTimer;

    static constexpr auto errorStrategy = std::to_array<const JobTimeoutConfig>(
        { { 30s, 2s }, { 60s, 5s }, { 150s, 30s } });
    int maxRetries = int(errorStrategy.size());
    int retriesTaken = 0;

    [[nodiscard]] const JobTimeoutConfig& getCurrentTimeoutConfig() const
    {
        return errorStrategy[std::min(size_t(retriesTaken),
                                      errorStrategy.size() - 1)];
    }

    [[nodiscard]] QString dumpRequest() const
    {
        static const std::array verbs { "GET"_ls, "PUT"_ls, "POST"_ls,
                                        "DELETE"_ls };
        const auto verbWord = verbs.at(size_t(verb));
        return verbWord % u' '
               % (reply ? reply->url().toString(QUrl::RemoveQuery)
                        : makeRequestUrl(connection->baseUrl(), apiEndpoint)
                              .toString());
    }
};

inline bool isHex(QChar c)
{
    return c.isDigit() || (c >= u'A' && c <= u'F') || (c >= u'a' && c <= u'f');
}

QByteArray BaseJob::encodeIfParam(const QString& paramPart)
{
    const auto percentIndex = paramPart.indexOf(u'%');
    if (percentIndex != -1 && paramPart.size() > percentIndex + 2
        && isHex(paramPart[percentIndex + 1])
        && isHex(paramPart[percentIndex + 2])) {
        qCWarning(JOBS)
            << "Developers, upfront percent-encoding of job parameters is "
               "deprecated since libQuotient 0.7; the string involved is"
            << paramPart;
        return QUrl(paramPart, QUrl::TolerantMode).toEncoded();
    }
    return QUrl::toPercentEncoding(paramPart);
}

BaseJob::BaseJob(HttpVerb verb, const QString& name, QByteArray endpoint,
                 bool needsToken)
    : BaseJob(verb, name, std::move(endpoint), QUrlQuery {}, RequestData {},
              needsToken)
{}

BaseJob::BaseJob(HttpVerb verb, const QString& name, QByteArray endpoint,
                 const QUrlQuery& query, RequestData&& data, bool needsToken)
    : d(makeImpl<Private>(verb, std::move(endpoint), query, std::move(data),
                          needsToken))
{
    setObjectName(name);
    connect(&d->timer, &QTimer::timeout, this, &BaseJob::timeout);
    connect(&d->retryTimer, &QTimer::timeout, this, [this] {
        qCDebug(d->logCat) << "Retrying" << this;
        d->connection->submit(this);
    });
}

BaseJob::~BaseJob()
{
    stop();
    d->retryTimer.stop(); // See #398
    qCDebug(d->logCat) << this << "destroyed";
}

QUrl BaseJob::requestUrl() const { return d->reply ? d->reply->url() : QUrl(); }

bool BaseJob::isBackground() const { return d->inBackground; }

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

QUrlQuery BaseJob::query() const { return d->requestQuery; }

void BaseJob::setRequestQuery(const QUrlQuery& query)
{
    d->requestQuery = query;
}

const RequestData& BaseJob::requestData() const { return d->requestData; }

void BaseJob::setRequestData(RequestData&& data)
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

QByteArrayList BaseJob::expectedKeys() const { return d->expectedKeys; }

void BaseJob::addExpectedKey(const QByteArray& key) { d->expectedKeys << key; }

void BaseJob::setExpectedKeys(const QByteArrayList& keys)
{
    d->expectedKeys = keys;
}

const QNetworkReply* BaseJob::reply() const { return d->reply.data(); }

QNetworkReply* BaseJob::reply() { return d->reply.data(); }

QUrl BaseJob::makeRequestUrl(QUrl baseUrl, const QByteArray& encodedPath,
                             const QUrlQuery& query)
{
    // Make sure the added path is relative even if it's not (the official
    // API definitions have the leading slash though it's not really correct).
    const auto pathUrl =
        QUrl::fromEncoded(encodedPath.mid(encodedPath.startsWith('/')),
                          QUrl::StrictMode);
    Q_ASSERT_X(pathUrl.isValid(), __FUNCTION__,
               qPrintable(pathUrl.errorString()));
    baseUrl = baseUrl.resolved(pathUrl);
    baseUrl.setQuery(query);
    return baseUrl;
}

QNetworkRequest BaseJob::Private::prepareRequest() const
{
    QNetworkRequest req{ makeRequestUrl(connection->baseUrl(), apiEndpoint,
                                        requestQuery) };
    if (!requestHeaders.contains("Content-Type"))
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"_ls);
    if (needsToken)
        req.setRawHeader("Authorization",
                         QByteArray("Bearer ") + connection->accessToken());
    req.setAttribute(QNetworkRequest::BackgroundRequestAttribute, inBackground);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setMaximumRedirectsAllowed(10);
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    // Qt doesn't combine HTTP2 with SSL quite right, occasionally crashing at
    // what seems like an attempt to write to a closed channel. If/when that
    // changes, false should be turned to true below.
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    Q_ASSERT(req.url().isValid());
    for (auto it = requestHeaders.cbegin(); it != requestHeaders.cend(); ++it)
        req.setRawHeader(it.key(), it.value());
    return req;
}

void BaseJob::Private::sendRequest(const QNetworkRequest& req)
{
    switch (verb) {
    case HttpVerb::Get:
        reply = connection->nam()->get(req);
        break;
    case HttpVerb::Post:
        reply = connection->nam()->post(req, requestData.source());
        break;
    case HttpVerb::Put:
        reply = connection->nam()->put(req, requestData.source());
        break;
    case HttpVerb::Delete:
        reply = connection->nam()->sendCustomRequest(req, "DELETE", requestData.source());
        break;
    }
}

void BaseJob::doPrepare() { }

void BaseJob::onSentRequest(QNetworkReply*) { }

void BaseJob::beforeAbandon() { }

void BaseJob::initiate(ConnectionData* connData, bool inBackground)
{
    if (Q_LIKELY(connData && connData->baseUrl().isValid())) {
        d->inBackground = inBackground;
        d->connection = connData;
        doPrepare();

        if (d->needsToken && d->connection->accessToken().isEmpty())
            setStatus(Unauthorised);
        else if ((d->verb == HttpVerb::Post || d->verb == HttpVerb::Put)
            && d->requestData.source()
            && !d->requestData.source()->isReadable()) {
            setStatus(FileError, "Request data not ready"_ls);
        }
        Q_ASSERT(status().code != Pending); // doPrepare() must NOT set this
        if (Q_LIKELY(status().code == Unprepared)) {
            d->promise.start();
            d->connection->submit(this);
            return;
        }
        qCWarning(d->logCat).noquote()
            << "Request failed preparation and won't be sent:"
            << d->dumpRequest();
    } else {
        qCCritical(d->logCat)
            << "Developers, ensure the Connection is valid before using it";
        setStatus(IncorrectRequest, tr("Invalid server connection"));
    }
    // The status is no good, finalise
    QTimer::singleShot(0, this, &BaseJob::finishJob);
}

void BaseJob::sendRequest()
{
    if (status().code == Abandoned) {
        // Normally sendRequest() shouldn't even be called on an abandoned job
        qWarning(d->logCat)
            << "Won't proceed with the abandoned request:" << d->dumpRequest();
        return;
    }
    Q_ASSERT(d->connection && status().code == Pending);
    d->needsToken |= d->connection->needsToken(objectName());
    auto req = d->prepareRequest();
    emit aboutToSendRequest(&req);
    d->sendRequest(req);
    Q_ASSERT(d->reply);
    connect(reply(), &QNetworkReply::finished, this, [this] {
        gotReply();
        finishJob();
    });
    if (d->reply->isRunning()) {
        connect(reply(), &QNetworkReply::metaDataChanged, this,
                [this] { checkReply(reply()); });
        connect(reply(), &QNetworkReply::uploadProgress, this,
                &BaseJob::uploadProgress);
        connect(reply(), &QNetworkReply::downloadProgress, this,
                &BaseJob::downloadProgress);
        d->timer.start(getCurrentTimeout());
        qDebug(d->logCat).noquote() << "Sent" << d->dumpRequest();
        onSentRequest(reply());
        emit sentRequest();
    } else
        qCritical(d->logCat).noquote()
            << "Request could not start:" << d->dumpRequest();
}

BaseJob::Status BaseJob::Private::parseJson()
{
    QJsonParseError error { 0, QJsonParseError::MissingObject };
    jsonResponse = QJsonDocument::fromJson(rawResponse, &error);
    return { error.error == QJsonParseError::NoError ? NoError
                                                     : IncorrectResponse,
             error.errorString() };
}

void BaseJob::gotReply()
{
    // Defer actually updating the status until it's finalised
    auto statusSoFar = checkReply(reply());
    if (statusSoFar.good()
        && d->expectedContentTypes == QByteArrayList { "application/json" }) //
    {
        d->rawResponse = reply()->readAll();
        statusSoFar = d->parseJson();
        if (statusSoFar.good() && !expectedKeys().empty()) {
            const auto& responseObject = jsonData();
            QByteArrayList missingKeys;
            for (const auto& k: expectedKeys())
                if (!responseObject.contains(QString::fromLatin1(k)))
                    missingKeys.push_back(k);
            if (!missingKeys.empty())
                statusSoFar = { IncorrectResponse,
                                tr("Required JSON keys missing: ")
                                    + QString::fromLatin1(missingKeys.join()) };
        }
        setStatus(statusSoFar);
        if (!status().good()) // Bad JSON in a "good" reply: bail out
            return;
        // If the endpoint expects anything else than just (API-related) JSON
        // reply()->readAll() is not performed and the whole reply processing
        // is left to derived job classes: they may read it piecemeal or customise
        // per content type in prepareResult(), or even have read it already
        // (see, e.g., DownloadFileJob).
    }
    if (statusSoFar.good()) {
        setStatus(prepareResult());
        return;
    }

    d->rawResponse = reply()->readAll();
    qCDebug(d->logCat).noquote()
        << "Error body (truncated if long):" << rawDataSample(500);
    setStatus(prepareError(statusSoFar));
}

bool checkContentType(const QByteArray& type, const QByteArrayList& patterns)
{
    if (patterns.isEmpty())
        return true;

    // ignore possible appendixes of the content type
    const auto ctype = type.split(';').front();

    for (const auto& pattern: patterns) {
        if (pattern.startsWith('*') || ctype == pattern) // Fast lane
            return true;

        auto patternParts = pattern.split('/');
        Q_ASSERT_X(patternParts.size() <= 2, __FUNCTION__,
                   qPrintable(
                       "BaseJob: Expected content type should have up to two /-separated parts; violating pattern: "_ls
                       + QString::fromLatin1(pattern)));

        if (ctype.split('/').front() == patternParts.front()
            && patternParts.back() == "*")
            return true; // Exact match already went on fast lane
    }

    return false;
}

BaseJob::Status BaseJob::checkReply(const QNetworkReply* reply) const
{
    // QNetworkReply error codes are insufficient for our purposes (e.g. they
    // don't allow to discern HTTP code 429) so check the original code instead
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
            qDebug(d->logCat).noquote() << httpCode << "<-" << d->dumpRequest();
        if (!checkContentType(reply->rawHeader("Content-Type"),
                              d->expectedContentTypes))
            return { UnexpectedResponseTypeWarning,
                     "Unexpected content type of the response"_ls };
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

BaseJob::Status BaseJob::prepareResult() { return Success; }

BaseJob::Status BaseJob::prepareError(Status currentStatus)
{
    // Try to make sense of the error payload but be prepared for all kinds
    // of unexpected stuff (raw HTML, plain text, foreign JSON among those)
    if (!d->rawResponse.isEmpty()
        && reply()->rawHeader("Content-Type") == "application/json")
        d->parseJson();

    // By now, if d->parseJson() above succeeded then jsonData() will return
    // a valid JSON object - or an empty object otherwise (in which case most
    // of if's below will fall through retaining the current status)
    const auto& errorJson = jsonData();
    const auto errCode = errorJson.value("errcode"_ls).toString();
    if (error() == TooManyRequests || errCode == "M_LIMIT_EXCEEDED"_ls) {
        QString msg = tr("Too many requests");
        int64_t retryAfterMs = errorJson.value("retry_after_ms"_ls).toInt(-1);
        if (retryAfterMs >= 0)
            msg += tr(", next retry advised after %1 ms").arg(retryAfterMs);
        else // We still have to figure some reasonable interval
            retryAfterMs = getNextRetryMs();

        d->connection->limitRate(milliseconds(retryAfterMs));

        return { TooManyRequests, msg };
    }

    if (errCode == "M_CONSENT_NOT_GIVEN"_ls) {
        d->errorUrl = QUrl(errorJson.value("consent_uri"_ls).toString());
        return { UserConsentRequired };
    }
    if (errCode == "M_UNSUPPORTED_ROOM_VERSION"_ls
        || errCode == "M_INCOMPATIBLE_ROOM_VERSION"_ls)
        return { UnsupportedRoomVersion,
                 errorJson.contains("room_version"_ls)
                     ? tr("Requested room version: %1")
                           .arg(errorJson.value("room_version"_ls).toString())
                     : errorJson.value("error"_ls).toString() };
    if (errCode == "M_CANNOT_LEAVE_SERVER_NOTICE_ROOM"_ls)
        return { CannotLeaveRoom,
                 tr("It's not allowed to leave a server notices room") };
    if (errCode == "M_USER_DEACTIVATED"_ls)
        return { UserDeactivated };

    // Not localisable on the client side
    if (errorJson.contains("error"_ls)) // Keep the code, update the message
        return { currentStatus.code, errorJson.value("error"_ls).toString() };

    return currentStatus; // The error payload is not recognised
}

QJsonValue BaseJob::takeValueFromJson(const QString& key)
{
    if (!d->jsonResponse.isObject())
        return QJsonValue::Undefined;
    auto o = d->jsonResponse.object();
    auto v = o.take(key);
    d->jsonResponse.setObject(o);
    return v;
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
    switch(error()) {
    case TooManyRequests:
        emit rateLimited();
        d->connection->submit(this);
        return;
    case Unauthorised:
        if (!d->needsToken && !d->connection->accessToken().isEmpty()) {
            // Rerun with access token (extension of the spec while
            // https://github.com/matrix-org/matrix-doc/issues/701 is pending)
            d->connection->setNeedsToken(objectName());
            qCWarning(d->logCat) << this << "re-running with authentication";
            emit retryScheduled(d->retriesTaken, 0);
            d->connection->submit(this);
            return;
        }
        break;
    case NetworkError:
    case IncorrectResponse:
    case Timeout:
        if (d->retriesTaken < d->maxRetries) {
            // TODO: The whole retrying thing should be put to
            // Connection(Manager) otherwise independently retrying jobs make a
            // bit of notification storm towards the UI.
            const seconds retryIn = error() == Timeout ? 0s
                                                       : getNextRetryInterval();
            ++d->retriesTaken;
            qCWarning(d->logCat).nospace()
                << this << ": retry #" << d->retriesTaken << " in "
                << retryIn.count() << " s";
            setStatus(Pending, "Pending retry"_ls);
            d->retryTimer.start(retryIn);
            emit retryScheduled(d->retriesTaken, milliseconds(retryIn).count());
            return;
        }
        [[fallthrough]];
    default:;
    }

    Q_ASSERT(status().code != Pending);

    // Notify those interested in any completion of the job including abandon()
    emit finished(this);

    d->promise.finish();
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

const QByteArray& BaseJob::rawData() const { return d->rawResponse; }

QString BaseJob::rawDataSample(int bytesAtMost) const
{
    auto data = QString::fromUtf8(rawData(bytesAtMost));
    Q_ASSERT(data.size() <= d->rawResponse.size());
    return data.size() == d->rawResponse.size()
               ? data
               : data + tr("...(truncated, %Ln bytes in total)",
                           "Comes after trimmed raw network response",
                           static_cast<int>(d->rawResponse.size()));
}

QJsonObject BaseJob::jsonData() const
{
    return d->jsonResponse.object();
}

QJsonArray BaseJob::jsonItems() const
{
    return d->jsonResponse.array();
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
    case Timeout:
        return tr("Request timed out");
    case Unauthorised:
        return tr("Unauthorised request");
    case ContentAccessError:
        return tr("Access error");
    case NotFound:
        return tr("Not found");
    case IncorrectRequest:
        return tr("Invalid request");
    case IncorrectResponse:
        return tr("Response could not be parsed");
    case TooManyRequests:
        return tr("Too many requests");
    case RequestNotImplemented:
        return tr("Function not implemented by the server");
    case NetworkAuthRequired:
        return tr("Network authentication required");
    case UserConsentRequired:
        return tr("User consent required");
    case UnsupportedRoomVersion:
        return tr("The server does not support the needed room version");
    default:
        return tr("Request failed");
    }
}

int BaseJob::error() const {
        return d->status.code; }

QString BaseJob::errorString() const {
        return d->status.message; }

QUrl BaseJob::errorUrl() const {
        return d->errorUrl; }

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
        s.message.replace(QString::fromUtf8(d->connection->accessToken()), "(REDACTED)"_ls);
    if (!s.good())
        qCWarning(d->logCat) << this << "status" << s;
    d->status = std::move(s);
    emit statusChanged(d->status);
}

void BaseJob::setStatus(int code, QString message)
{
    setStatus({ code, std::move(message) });
}

void BaseJob::forceResult(QJsonDocument resultDoc, Status s)
{
    d->jsonResponse = std::move(resultDoc);
    setStatus(std::move(s));
    QMetaObject::invokeMethod(this, [this] { finishJob(); }, Qt::QueuedConnection);
}

void BaseJob::abandon()
{
    beforeAbandon();
    d->timer.stop();
    d->retryTimer.stop(); // In case abandon() was called between retries
    setStatus(Abandoned);
    if (d->reply)
        d->reply->disconnect(this);
    emit finished(this);

    deleteLater(); // The promise will cancel itself on deletion
}

void BaseJob::timeout()
{
    setStatus(Timeout, "The job has timed out"_ls);
    finishJob();
}

void BaseJob::setLoggingCategory(QMessageLogger::CategoryFunction lcf)
{
    d->logCat = lcf;
}

QFuture<void> BaseJob::future() { return d->promise.future(); }

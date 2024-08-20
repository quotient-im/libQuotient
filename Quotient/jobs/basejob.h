// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "requestdata.h"

#include <QtCore/QObject>
#include <QtCore/QStringBuilder>
#include <QtCore/QLoggingCategory>
#include <QtCore/QFuture>

#include <Quotient/converters.h> // Common for csapi/ headers even though not used here
#include <Quotient/quotient_common.h> // For DECL_DEPRECATED_ENUMERATOR

class QNetworkRequest;
class QNetworkReply;
class QSslError;

namespace Quotient {
class ConnectionData;

enum class HttpVerb { Get, Put, Post, Delete };

class QUOTIENT_API BaseJob : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl requestUrl READ requestUrl CONSTANT)
    Q_PROPERTY(int maxRetries READ maxRetries WRITE setMaxRetries)
    Q_PROPERTY(int statusCode READ error NOTIFY statusChanged)

    static QByteArray encodeIfParam(const QString& paramPart);
    template <int N>
    static auto encodeIfParam(const char (&constPart)[N])
    {
        return constPart;
    }

public:
    //! \brief Job status codes
    //!
    //! Every job is created in Unprepared status; upon calling Connection::prepare(), if things are
    //! fine, it becomes Pending and remains so until the reply arrives; then the status code is
    //! set according to the job result. At any point in time the job can be abandon()ed, causing
    //! it to become Abandoned for a brief period before deletion.
    enum StatusCode {
        Success = 0,
        NoError = Success,
        Pending = 1,
        WarningLevel = 20, //!< Warnings have codes starting from this
        UnexpectedResponseType = 21,
        UnexpectedResponseTypeWarning = UnexpectedResponseType,
        Unprepared = 25, //!< Initial job state is incomplete, hence warning level
        Abandoned = 50, //!< A tiny period between abandoning and object deletion
        ErrorLevel = 100, //!< Errors have codes starting from this
        NetworkError = 101,
        Timeout,
        Unauthorised,
        ContentAccessError,
        NotFound,
        IncorrectRequest,
        IncorrectResponse,
        TooManyRequests,
        RateLimited = TooManyRequests,
        RequestNotImplemented,
        UnsupportedRoomVersion,
        NetworkAuthRequired,
        UserConsentRequired,
        CannotLeaveRoom,
        UserDeactivated,
        FileError,
        UserDefinedError = 256
    };
    Q_ENUM(StatusCode)

    template <typename... StrTs>
    static QByteArray makePath(StrTs&&... parts)
    {
        return (QByteArray() % ... % encodeIfParam(parts));
    }

    //! \brief The status of a job
    //!
    //! The status consists of a code that is described (but not delimited) by StatusCode, and
    //! a freeform message.
    //!
    //! To extend the list of error codes, define an (anonymous) enum along the lines of StatusCode,
    //! with additional values starting at UserDefinedError.
    struct Status {
        Status(StatusCode c) : code(c) {}
        Status(int c, QString m) : code(c), message(std::move(m)) {}

        static StatusCode fromHttpCode(int httpCode);
        static Status fromHttpCode(int httpCode, QString msg)
        {
            return { fromHttpCode(httpCode), std::move(msg) };
        }

        bool good() const { return code < ErrorLevel; }
        QDebug dumpToLog(QDebug dbg) const;
        friend QDebug operator<<(const QDebug& dbg, const Status& s)
        {
            return s.dumpToLog(dbg);
        }

        bool operator==(const Status& other) const
        {
            return code == other.code && message == other.message;
        }
        bool operator!=(const Status& other) const
        {
            return !operator==(other);
        }
        bool operator==(int otherCode) const
        {
            return code == otherCode;
        }
        bool operator!=(int otherCode) const
        {
            return !operator==(otherCode);
        }

        int code;
        QString message;
    };

public:
    BaseJob(HttpVerb verb, const QString& name, QByteArray endpoint,
            bool needsToken = true);
    BaseJob(HttpVerb verb, const QString& name, QByteArray endpoint,
            const QUrlQuery& query, RequestData&& data = {},
            bool needsToken = true);

    QUrl requestUrl() const;
    bool isBackground() const;

    //! Current status of the job
    Status status() const;

    //! Short human-friendly message on the job status
    QString statusCaption() const;

    //! \brief Get first bytes of the raw response body as received from the server
    //! \param bytesAtMost the number of leftmost bytes to return
    //! \sa rawDataSample
    QByteArray rawData(int bytesAtMost) const;

    //! Access the whole response body as received from the server
    const QByteArray& rawData() const;

    //! \brief Get UI-friendly sample of raw data
    //!
    //! This is almost the same as rawData but appends the "truncated" suffix if not all data fit in
    //! bytesAtMost. This call is recommended to present a sample of raw data as "details" next to
    //! error messages. Note that the default \p bytesAtMost value is also tailored to UI cases.
    //!
    //! \sa //! rawData
    QString rawDataSample(int bytesAtMost = 65535) const;

    //! \brief Get the response body as a JSON object
    //!
    //! If the job's returned content type is not `application/json` or if the top-level JSON entity
    //! is not an object, an empty object is returned.
    QJsonObject jsonData() const;

    //! \brief Get the response body as a JSON array
    //!
    //! If the job's returned content type is not `application/json` or if the top-level JSON entity
    //! is not an array, an empty array is returned.
    QJsonArray jsonItems() const;

    //! \brief Load the property from the JSON response assuming a given C++ type
    //!
    //! If there's no top-level JSON object in the response or if there's
    //! no node with the key \p keyName, \p defaultValue is returned.
    template <typename T, typename StrT>
    T loadFromJson(const StrT& keyName, T&& defaultValue = {}) const
    {
        const auto& jv = jsonData().value(keyName);
        return jv.isUndefined() ? std::forward<T>(defaultValue)
                                : fromJson<T>(jv);
    }

    //! \brief Load the property from the JSON response and delete it from JSON
    //!
    //! If there's no top-level JSON object in the response or if there's
    //! no node with the key \p keyName, \p defaultValue is returned.
    template <typename T>
    T takeFromJson(const QString& key, T&& defaultValue = {})
    {
        if (const auto& jv = takeValueFromJson(key); !jv.isUndefined())
            return fromJson<T>(jv);

        return std::forward<T>(defaultValue);
    }

    //! \brief Error (more generally, status) code
    //!
    //! Equivalent to status().code
    //! \sa status, StatusCode
    int error() const;

    //! Error-specific message, as returned by the server
    virtual QString errorString() const;

    //! A URL to help/clarify the error, if provided by the server
    QUrl errorUrl() const;

    int maxRetries() const;
    void setMaxRetries(int newMaxRetries);

    using duration_ms_t = std::chrono::milliseconds::rep; // normally int64_t

    std::chrono::seconds getCurrentTimeout() const;
    Q_INVOKABLE Quotient::BaseJob::duration_ms_t getCurrentTimeoutMs() const;
    std::chrono::seconds getNextRetryInterval() const;
    Q_INVOKABLE Quotient::BaseJob::duration_ms_t getNextRetryMs() const;
    std::chrono::milliseconds timeToRetry() const;
    Q_INVOKABLE Quotient::BaseJob::duration_ms_t millisToRetry() const;

    friend QDebug operator<<(QDebug dbg, const BaseJob* j)
    {
        return dbg << j->objectName();
    }

public Q_SLOTS:
    void initiate(Quotient::ConnectionData* connData, bool inBackground);

    //! \brief Abandon the result of this job, arrived or unarrived.
    //!
    //! This aborts waiting for a reply from the server (if there was
    //! any pending) and deletes the job object. No result signals
    //! (result, success, failure) are emitted, only finished() is.
    void abandon();

Q_SIGNALS:
    //! \brief The job is about to send a network request
    //!
    //! This signal is emitted every time a network request is made (which can
    //! occur several times due to job retries). You can use it to change
    //! the request parameters (such as redirect policy) if necessary. If you
    //! need to set additional request headers or query items, do that using
    //! setRequestHeaders() and setRequestQuery() instead.
    //! \note \p req is not guaranteed to exist (i.e. it may point to garbage)
    //!       unless this signal is handled via a DirectConnection (or
    //!       BlockingQueuedConnection if in another thread), i.e.,
    //!       synchronously.
    //! \sa setRequestHeaders, setRequestQuery
    void aboutToSendRequest(QNetworkRequest* req);

    //! The job has sent a network request
    void sentRequest();

    //! The job has changed its status
    void statusChanged(Quotient::BaseJob::Status newStatus);

    //! \brief A retry of the network request is scheduled after the previous request failed
    //! \param nextAttempt the 1-based number of attempt (will always be more than 1)
    //! \param inMilliseconds the interval after which the next attempt will be taken
    void retryScheduled(int nextAttempt, Quotient::BaseJob::duration_ms_t inMilliseconds);

    //! \brief The job has been rate-limited
    //!
    //! The previous network request has been rate-limited; the next attempt will be queued and run
    //! sometime later. Since other jobs may already wait in the queue, it's not possible to predict
    //! the wait time.
    void rateLimited();

    //! \brief The job has finished - either with a result, or abandoned
    //!
    //! Emitted when the job is finished, in any case. It is used to notify
    //! observers that the job is terminated and that progress can be hidden.
    //!
    //! This should not be emitted directly by subclasses; use finishJob() instead.
    //!
    //! In general, to be notified of a job's completion, client code should connect to result(),
    //! success(), or failure() rather than finished(). However if you need to track the job's
    //! lifecycle you should connect to this instead of result(); in particular, only this signal
    //! will be emitted on abandoning, the others won't.
    //!
    //! \param job the job that emitted this signal
    //!
    //! \sa result, success, failure
    void finished(Quotient::BaseJob* job);

    //! \brief The job has finished with a result, successful or unsuccessful
    //!
    //! Use error() or status().good() to know if the job has finished successfully.
    //!
    //! \param job the job that emitted this signal
    //!
    //! \sa success, failure
    void result(Quotient::BaseJob* job);

    //! \brief The job has finished with a successful result
    //! \sa result, failure
    void success(Quotient::BaseJob*);

    //! \brief The job has finished with a failure result
    //! Emitted together with result() when the job resulted in an error. Mutually exclusive with
    //! success(): after result() is emitted, exactly one of success() and failure() will be emitted
    //! next. Will not be emitted in case of abandon()ing.
    //!
    //! \sa result, success
    void failure(Quotient::BaseJob*);

    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

protected:
    using headers_t = QHash<QByteArray, QByteArray>;

    QByteArray apiEndpoint() const;
    void setApiEndpoint(QByteArray apiEndpoint);
    const headers_t& requestHeaders() const;
    void setRequestHeader(const headers_t::key_type& headerName,
                          const headers_t::mapped_type& headerValue);
    void setRequestHeaders(const headers_t& headers);
    QUrlQuery query() const;
    void setRequestQuery(const QUrlQuery& query);
    const RequestData& requestData() const;
    void setRequestData(RequestData&& data);
    const QByteArrayList& expectedContentTypes() const;
    void addExpectedContentType(const QByteArray& contentType);
    void setExpectedContentTypes(const QByteArrayList& contentTypes);
    QByteArrayList expectedKeys() const;
    void addExpectedKey(const QByteArray &key);
    void setExpectedKeys(const QByteArrayList &keys);

    const QNetworkReply* reply() const;
    QNetworkReply* reply();

    //! \brief Construct a URL out of baseUrl, path and query
    //!
    //! The function ensures exactly one '/' between the path component of
    //! \p baseUrl and \p path. The query component of \p baseUrl is ignored.
    //! \note Unlike most of BaseJob, this function is thread-safe
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QByteArray& encodedPath,
                               const QUrlQuery& query = {});

    //! \brief Prepare the job for execution
    //!
    //! This method is called no more than once per job lifecycle, when it's first scheduled
    //! for execution; in particular, it is not called on retries.
    virtual void doPrepare(const ConnectionData*);

    //! \brief Postprocessing after the network request has been sent
    //!
    //! This method is called every time the job receives a running
    //! QNetworkReply object from NetworkAccessManager - basically, after
    //! successfully sending a network request (including retries).
    virtual void onSentRequest(QNetworkReply*);

    virtual void beforeAbandon();

    //! \brief Check the pending or received reply for upfront issues
    //!
    //! This is invoked when headers are first received and also once the complete reply is
    //! obtained; the base implementation checks the HTTP headers to detect general issues such as
    //! network errors or access denial and it's strongly recommended to call it from overrides, as
    //! early as possible.
    //!
    //! This slot is const and cannot read the response body from the reply. If you need to read the
    //! body on the fly, override onSentRequest() and connect in it to reply->readyRead(); and if
    //! you only need to validate the body after it fully arrived, use prepareResult() for that.
    //! Returning anything except NoError/Success switches further processing from prepareResult()
    //! to prepareError().
    //!
    //! \return the result of checking the reply
    //!
    //! \sa gotReply
    virtual Status checkReply(const QNetworkReply* reply) const;

    //! \brief An extension point for additional reply processing
    //!
    //! The base implementation simply returns Success without doing anything else.
    //!
    //! \sa gotReply
    virtual Status prepareResult();

    //! \brief Process details of the error
    //!
    //! The function processes the reply in case when status from checkReply() was not good (usually
    //! because of an unsuccessful HTTP code). The base implementation assumes Matrix JSON error
    //! object in the body; overrides are strongly recommended to call it for all stock Matrix
    //! responses as early as possible and only then process custom errors, with JSON or non-JSON
    //! payload.
    //!
    //! \return updated (if necessary) job status
    virtual Status prepareError(Status currentStatus);

    //! \brief Retrieve a value for one specific key and delete it from the JSON response object
    //!
    //! This allows to implement deserialisation with "move" semantics for parts
    //! of the response. Assuming that the response body is a valid JSON object,
    //! the function calls QJsonObject::take(key) on it and returns the result.
    //!
    //! \return QJsonValue::Undefined if the response content is not a JSON object or it doesn't
    //!         have \p key; the value for \p key otherwise.
    //!
    //! \sa takeFromJson
    QJsonValue takeValueFromJson(const QString& key);

    void setStatus(Status s);
    void setStatus(int code, QString message);

    //! \brief Force completion of the job for sake of testing
    //!
    //! Normal jobs should never use; this is only meant to be used in test mocks.
    //! \sa Mocked
    void forceResult(QJsonDocument resultDoc, Status s = { Success });

    //! \brief Set the logging category for the given job instance
    //!
    //! \param lcf The logging category function to provide the category -
    //!            the one you define with Q_LOGGING_CATEGORY (without
    //!            parentheses, BaseJob will call it for you)
    void setLoggingCategory(QMessageLogger::CategoryFunction lcf);

    // Job objects should only be deleted via QObject::deleteLater
    ~BaseJob() override;

protected Q_SLOTS:
    void timeout();

private Q_SLOTS:
    void sendRequest();
    void gotReply();

private:
    friend class ConnectionData; // to provide access to sendRequest()
    template <class JobT>
    friend class JobHandle;

    void stop();
    void finishJob();
    QFuture<void> future();

    class Private;
    ImplPtr<Private> d;
};

inline bool QUOTIENT_API isJobPending(BaseJob* job)
{
    return job && job->error() == BaseJob::Pending;
}

template <typename JobT>
constexpr inline auto doCollectResponse = nullptr;

//! \brief Get a job response in a single structure
//!
//! Use this to get all parts of the job response in a single C++ type, defined by the job class.
//! It can be either an aggregate of individual response parts returned by job accessors, or, if
//! the response is already singular, a type of this response. The default implementation caters to
//! generated jobs, where the code has to be generic enough to work with copyable and movable-only
//! responses. For manually written code, simply overload collectResponse() for the respective
//! job type, with appropriate constness.
template <std::derived_from<BaseJob> JobT>
inline auto collectResponse(JobT* job)
    requires requires { doCollectResponse<JobT>(job); }
{
    return doCollectResponse<JobT>(job);
}

template <std::derived_from<BaseJob> JobT>
class Mocked : public JobT {
public:
    using JobT::JobT;
    void setResult(QJsonDocument d) { JobT::forceResult(std::move(d)); }
};

} // namespace Quotient

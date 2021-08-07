// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "requestdata.h"
#include "../logging.h"
#include "../converters.h"

#include <QtCore/QObject>

class QNetworkReply;
class QSslError;

namespace Quotient {
class ConnectionData;

enum class HttpVerb { Get, Put, Post, Delete };

class BaseJob : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl requestUrl READ requestUrl CONSTANT)
    Q_PROPERTY(int maxRetries READ maxRetries WRITE setMaxRetries)
    Q_PROPERTY(int statusCode READ error NOTIFY statusChanged)
public:
    /*! The status code of a job
     *
     * Every job is created in Unprepared status; upon calling prepare()
     * from Connection (if things are fine) it go to Pending status. After
     * that, the next transition comes after the reply arrives and its contents
     * are analysed. At any point in time the job can be abandon()ed, causing
     * it to switch to status Abandoned for a brief period before deletion.
     */
    enum StatusCode {
        Success = 0,
        NoError = Success, // To be compatible with Qt conventions
        Pending = 1,
        WarningLevel = 20, //< Warnings have codes starting from this
        UnexpectedResponseType = 21,
        UnexpectedResponseTypeWarning = UnexpectedResponseType,
        Unprepared = 25, //< Initial job state is incomplete, hence warning level
        Abandoned = 50, //< A tiny period between abandoning and object deletion
        ErrorLevel = 100, //< Errors have codes starting from this
        NetworkError = 101,
        Timeout,
        TimeoutError = Timeout,
        Unauthorised,
        ContentAccessError,
        NotFoundError,
        IncorrectRequest,
        IncorrectRequestError = IncorrectRequest,
        IncorrectResponse,
        IncorrectResponseError = IncorrectResponse,
        JsonParseError //< \deprecated Use IncorrectResponse instead
        = IncorrectResponse,
        TooManyRequests,
        TooManyRequestsError = TooManyRequests,
        RateLimited = TooManyRequests,
        RequestNotImplemented,
        RequestNotImplementedError = RequestNotImplemented,
        UnsupportedRoomVersion,
        UnsupportedRoomVersionError = UnsupportedRoomVersion,
        NetworkAuthRequired,
        NetworkAuthRequiredError = NetworkAuthRequired,
        UserConsentRequired,
        UserConsentRequiredError = UserConsentRequired,
        CannotLeaveRoom,
        UserDeactivated,
        FileError,
        UserDefinedError = 256
    };
    Q_ENUM(StatusCode)

    using Data = RequestData;

    /*!
     * This structure stores the status of a server call job. The status
     * consists of a code, that is described (but not delimited) by the
     * respective enum, and a freeform message.
     *
     * To extend the list of error codes, define an (anonymous) enum
     * along the lines of StatusCode, with additional values
     * starting at UserDefinedError
     */
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
    BaseJob(HttpVerb verb, const QString& name, const QString& endpoint,
            bool needsToken = true);
    BaseJob(HttpVerb verb, const QString& name, const QString& endpoint,
            const QUrlQuery& query, Data&& data = {}, bool needsToken = true);

    QUrl requestUrl() const;
    bool isBackground() const;

    /** Current status of the job */
    Status status() const;

    /** Short human-friendly message on the job status */
    QString statusCaption() const;

    /*! Get first bytes of the raw response body as received from the server
     *
     * \param bytesAtMost the number of leftmost bytes to return
     *
     * \sa rawDataSample
     */
    QByteArray rawData(int bytesAtMost) const;

    /*! Access the whole response body as received from the server */
    const QByteArray& rawData() const;

    /** Get UI-friendly sample of raw data
     *
     * This is almost the same as rawData but appends the "truncated"
     * suffix if not all data fit in bytesAtMost. This call is
     * recommended to present a sample of raw data as "details" next to
     * error messages. Note that the default \p bytesAtMost value is
     * also tailored to UI cases.
     *
     * \sa rawData
     */
    QString rawDataSample(int bytesAtMost = 65535) const;

    /** Get the response body as a JSON object
     *
     * If the job's returned content type is not `application/json`
     * or if the top-level JSON entity is not an object, an empty object
     * is returned.
     */
    QJsonObject jsonData() const;

    /** Get the response body as a JSON array
     *
     * If the job's returned content type is not `application/json`
     * or if the top-level JSON entity is not an array, an empty array
     * is returned.
     */
    QJsonArray jsonItems() const;

    /** Load the property from the JSON response assuming a given C++ type
     *
     * If there's no top-level JSON object in the response or if there's
     * no node with the key \p keyName, \p defaultValue is returned.
     */
    template <typename T, typename StrT>
    T loadFromJson(const StrT& keyName, T&& defaultValue = {}) const
    {
        const auto& jv = jsonData().value(keyName);
        return jv.isUndefined() ? std::forward<T>(defaultValue)
                                : fromJson<T>(jv);
    }

    /** Load the property from the JSON response and delete it from JSON
     *
     * If there's no top-level JSON object in the response or if there's
     * no node with the key \p keyName, \p defaultValue is returned.
     */
    template <typename T>
    T takeFromJson(const QString& key, T&& defaultValue = {})
    {
        if (const auto& jv = takeValueFromJson(key); !jv.isUndefined())
            return fromJson<T>(jv);

        return std::forward<T>(defaultValue);
    }

    /** Error (more generally, status) code
     * Equivalent to status().code
     * \sa status
     */
    int error() const;

    /** Error-specific message, as returned by the server */
    virtual QString errorString() const;

    /** A URL to help/clarify the error, if provided by the server */
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
    void initiate(ConnectionData* connData, bool inBackground);

    /**
     * Abandons the result of this job, arrived or unarrived.
     *
     * This aborts waiting for a reply from the server (if there was
     * any pending) and deletes the job object. No result signals
     * (result, success, failure) are emitted.
     */
    void abandon();

Q_SIGNALS:
    /** The job is about to send a network request */
    void aboutToSendRequest();

    /** The job has sent a network request */
    void sentRequest();

    /** The job has changed its status */
    void statusChanged(Quotient::BaseJob::Status newStatus);

    /**
     * The previous network request has failed; the next attempt will
     * be done in the specified time
     * @param nextAttempt the 1-based number of attempt (will always be more
     * than 1)
     * @param inMilliseconds the interval after which the next attempt will be
     * taken
     */
    void retryScheduled(int nextAttempt,
                        Quotient::BaseJob::duration_ms_t inMilliseconds);

    /**
     * The previous network request has been rate-limited; the next attempt
     * will be queued and run sometime later. Since other jobs may already
     * wait in the queue, it's not possible to predict the wait time.
     */
    void rateLimited();

    /**
     * Emitted when the job is finished, in any case. It is used to notify
     * observers that the job is terminated and that progress can be hidden.
     *
     * This should not be emitted directly by subclasses;
     * use finishJob() instead.
     *
     * In general, to be notified of a job's completion, client code
     * should connect to result(), success(), or failure()
     * rather than finished(). However if you need to track the job's
     * lifecycle you should connect to this instead of result();
     * in particular, only this signal will be emitted on abandoning.
     *
     * @param job the job that emitted this signal
     *
     * @see result, success, failure
     */
    void finished(Quotient::BaseJob* job);

    /**
     * Emitted when the job is finished (except when abandoned).
     *
     * Use error() to know if the job was finished with error.
     *
     * @param job the job that emitted this signal
     *
     * @see success, failure
     */
    void result(Quotient::BaseJob* job);

    /**
     * Emitted together with result() in case there's no error.
     *
     * @see result, failure
     */
    void success(Quotient::BaseJob*);

    /**
     * Emitted together with result() if there's an error.
     * Similar to result(), this won't be emitted in case of abandon().
     *
     * @see result, success
     */
    void failure(Quotient::BaseJob*);

    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

protected:
    using headers_t = QHash<QByteArray, QByteArray>;

    const QString& apiEndpoint() const;
    void setApiEndpoint(const QString& apiEndpoint);
    const headers_t& requestHeaders() const;
    void setRequestHeader(const headers_t::key_type& headerName,
                          const headers_t::mapped_type& headerValue);
    void setRequestHeaders(const headers_t& headers);
    const QUrlQuery& query() const;
    void setRequestQuery(const QUrlQuery& query);
    const Data& requestData() const;
    void setRequestData(Data&& data);
    const QByteArrayList& expectedContentTypes() const;
    void addExpectedContentType(const QByteArray& contentType);
    void setExpectedContentTypes(const QByteArrayList& contentTypes);
    QByteArrayList expectedKeys() const;
    void addExpectedKey(const QByteArray &key);
    void setExpectedKeys(const QByteArrayList &keys);

    const QNetworkReply* reply() const;
    QNetworkReply* reply();

    /** Construct a URL out of baseUrl, path and query
     *
     * The function ensures exactly one '/' between the path component of
     * \p baseUrl and \p path. The query component of \p baseUrl is ignored.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& path,
                               const QUrlQuery& query = {});

    /*! Prepares the job for execution
     *
     * This method is called no more than once per job lifecycle,
     * when it's first scheduled for execution; in particular, it is not called
     * on retries.
     */
    virtual void doPrepare();

    /*! Postprocessing after the network request has been sent
     *
     * This method is called every time the job receives a running
     * QNetworkReply object from NetworkAccessManager - basically, after
     * successfully sending a network request (including retries).
     */
    virtual void onSentRequest(QNetworkReply*);
    virtual void beforeAbandon();

    /*! \brief An extension point for additional reply processing.
     *
     * The base implementation does nothing and returns Success.
     *
     * \sa gotReply
     */
    virtual Status prepareResult();

    /*! \brief Process details of the error
     *
     * The function processes the reply in case when status from checkReply()
     * was not good (usually because of an unsuccessful HTTP code).
     * The base implementation assumes Matrix JSON error object in the body;
     * overrides are strongly recommended to call it for all stock Matrix
     * responses as early as possible but in addition can process custom errors,
     * with JSON or non-JSON payload.
     */
    virtual Status prepareError();

    /*! \brief Get direct access to the JSON response object in the job
     *
     * This allows to implement deserialisation with "move" semantics for parts
     * of the response. Assuming that the response body is a valid JSON object,
     * the function calls QJsonObject::take(key) on it and returns the result.
     *
     * \return QJsonValue::Null, if the response content type is not
     *                           advertised as `application/json`;
     *         QJsonValue::Undefined, if the response is a JSON object but
     *                                doesn't have \p key;
     *         the value for \p key otherwise.
     *
     * \sa takeFromJson
     */
    QJsonValue takeValueFromJson(const QString& key);

    void setStatus(Status s);
    void setStatus(int code, QString message);

    // Q_DECLARE_LOGGING_CATEGORY return different function types
    // in different versions
    using LoggingCategory = decltype(JOBS)*;
    void setLoggingCategory(LoggingCategory lcf);

    // Job objects should only be deleted via QObject::deleteLater
    ~BaseJob() override;

protected Q_SLOTS:
    void timeout();

    /*! \brief Check the pending or received reply for upfront issues
     *
     * This is invoked when headers are first received and also once
     * the complete reply is obtained; the base implementation checks the HTTP
     * headers to detect general issues such as network errors or access denial
     * and it's strongly recommended to call it from overrides,
     * as early as possible.
     * This slot is const and cannot read the response body. If you need to read
     * the body on the fly, override onSentRequest() and connect in it
     * to reply->readyRead(); and if you only need to validate the body after
     * it fully arrived, use prepareResult() for that). Returning anything
     * except NoError/Success switches further processing from prepareResult()
     * to prepareError().
     *
     * @return the result of checking the reply
     *
     * @see gotReply
     */
    virtual Status checkReply(const QNetworkReply *reply) const;

private Q_SLOTS:
    void sendRequest();
    void gotReply();

    friend class ConnectionData; // to provide access to sendRequest()

private:
    void stop();
    void finishJob();

    class Private;
    QScopedPointer<Private> d;
};

inline bool isJobPending(BaseJob* job)
{
    return job && job->error() == BaseJob::Pending;
}
} // namespace Quotient

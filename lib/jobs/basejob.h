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

#pragma once

#include "../logging.h"
#include "requestdata.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QObject>
#include <QtCore/QUrlQuery>

class QNetworkReply;
class QSslError;

namespace QMatrixClient
{
class ConnectionData;

enum class HttpVerb
{
    Get,
    Put,
    Post,
    Delete
};

struct JobTimeoutConfig
{
    int jobTimeout;
    int nextRetryInterval;
};

class BaseJob : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl requestUrl READ requestUrl CONSTANT)
    Q_PROPERTY(int maxRetries READ maxRetries WRITE setMaxRetries)
public:
    enum StatusCode
    {
        NoError = 0 // To be compatible with Qt conventions
        ,
        Success = 0,
        Pending = 1,
        WarningLevel = 20,
        UnexpectedResponseType = 21,
        UnexpectedResponseTypeWarning = UnexpectedResponseType,
        Abandoned = 50 //< A tiny period between abandoning and object deletion
        ,
        ErrorLevel = 100 //< Errors have codes starting from this
        ,
        NetworkError = 100,
        Timeout,
        TimeoutError = Timeout,
        ContentAccessError,
        NotFoundError,
        IncorrectRequest,
        IncorrectRequestError = IncorrectRequest,
        IncorrectResponse,
        IncorrectResponseError = IncorrectResponse,
        JsonParseError //< deprecated; Use IncorrectResponse instead
        = IncorrectResponse,
        TooManyRequests,
        TooManyRequestsError = TooManyRequests,
        RequestNotImplemented,
        RequestNotImplementedError = RequestNotImplemented,
        UnsupportedRoomVersion,
        UnsupportedRoomVersionError = UnsupportedRoomVersion,
        NetworkAuthRequired,
        NetworkAuthRequiredError = NetworkAuthRequired,
        UserConsentRequired,
        UserConsentRequiredError = UserConsentRequired,
        UserDefinedError = 256
    };

    /**
     * A simple wrapper around QUrlQuery that allows its creation from
     * a list of string pairs
     */
    class Query : public QUrlQuery
    {
    public:
        using QUrlQuery::QUrlQuery;
        Query() = default;
        Query(const std::initializer_list<QPair<QString, QString>>& l)
        {
            setQueryItems(l);
        }
    };

    using Data = RequestData;

    /**
     * This structure stores the status of a server call job. The status
     * consists of a code, that is described (but not delimited) by the
     * respective enum, and a freeform message.
     *
     * To extend the list of error codes, define an (anonymous) enum
     * along the lines of StatusCode, with additional values
     * starting at UserDefinedError
     */
    class Status
    {
    public:
        Status(StatusCode c)
            : code(c)
        {}
        Status(int c, QString m)
            : code(c)
            , message(std::move(m))
        {}

        bool good() const { return code < ErrorLevel; }
        friend QDebug operator<<(QDebug dbg, const Status& s)
        {
            QDebugStateSaver _s(dbg);
            return dbg.noquote().nospace() << s.code << ": " << s.message;
        }

        bool operator==(const Status& other) const
        {
            return code == other.code && message == other.message;
        }
        bool operator!=(const Status& other) const
        {
            return !operator==(other);
        }

        int code;
        QString message;
    };

    using duration_t = int; // milliseconds

public:
    BaseJob(HttpVerb verb, const QString& name, const QString& endpoint,
            bool needsToken = true);
    BaseJob(HttpVerb verb, const QString& name, const QString& endpoint,
            const Query& query, Data&& data = {}, bool needsToken = true);

    QUrl requestUrl() const;
    bool isBackground() const;

    /** Current status of the job */
    Status status() const;
    /** Short human-friendly message on the job status */
    QString statusCaption() const;
    /** Get raw response body as received from the server
     * \param bytesAtMost return this number of leftmost bytes, or -1
     *                    to return the entire response
     */
    QByteArray rawData(int bytesAtMost = -1) const;
    /** Get UI-friendly sample of raw data
     *
     * This is almost the same as rawData but appends the "truncated"
     * suffix if not all data fit in bytesAtMost. This call is
     * recommended to present a sample of raw data as "details" next to
     * error messages. Note that the default \p bytesAtMost value is
     * also tailored to UI cases.
     */
    QString rawDataSample(int bytesAtMost = 65535) const;

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

    Q_INVOKABLE duration_t getCurrentTimeout() const;
    Q_INVOKABLE duration_t getNextRetryInterval() const;
    Q_INVOKABLE duration_t millisToRetry() const;

    friend QDebug operator<<(QDebug dbg, const BaseJob* j)
    {
        return dbg << j->objectName();
    }

public slots:
    void start(const ConnectionData* connData, bool inBackground = false);

    /**
     * Abandons the result of this job, arrived or unarrived.
     *
     * This aborts waiting for a reply from the server (if there was
     * any pending) and deletes the job object. No result signals
     * (result, success, failure) are emitted.
     */
    void abandon();

signals:
    /** The job is about to send a network request */
    void aboutToStart();

    /** The job has sent a network request */
    void started();

    /** The job has changed its status */
    void statusChanged(Status newStatus);

    /**
     * The previous network request has failed; the next attempt will
     * be done in the specified time
     * @param nextAttempt the 1-based number of attempt (will always be more
     * than 1)
     * @param inMilliseconds the interval after which the next attempt will be
     * taken
     */
    void retryScheduled(int nextAttempt, int inMilliseconds);

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
    void finished(BaseJob* job);

    /**
     * Emitted when the job is finished (except when abandoned).
     *
     * Use error() to know if the job was finished with error.
     *
     * @param job the job that emitted this signal
     *
     * @see success, failure
     */
    void result(BaseJob* job);

    /**
     * Emitted together with result() in case there's no error.
     *
     * @see result, failure
     */
    void success(BaseJob*);

    /**
     * Emitted together with result() if there's an error.
     * Similar to result(), this won't be emitted in case of abandon().
     *
     * @see result, success
     */
    void failure(BaseJob*);

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

    /** Construct a URL out of baseUrl, path and query
     * The function automatically adds '/' between baseUrl's path and
     * \p path if necessary. The query component of \p baseUrl
     * is ignored.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& path,
                               const QUrlQuery& query = {});

    virtual void beforeStart(const ConnectionData* connData);
    virtual void afterStart(const ConnectionData* connData,
                            QNetworkReply* reply);
    virtual void beforeAbandon(QNetworkReply*);

    /**
     * Used by gotReply() to check the received reply for general
     * issues such as network errors or access denial.
     * Returning anything except NoError/Success prevents
     * further parseReply()/parseJson() invocation.
     *
     * @param reply the reply received from the server
     * @return the result of checking the reply
     *
     * @see gotReply
     */
    virtual Status doCheckReply(QNetworkReply* reply) const;

    /**
     * Processes the reply. By default, parses the reply into
     * a QJsonDocument and calls parseJson() if it's a valid JSON.
     *
     * @param reply raw contents of a HTTP reply from the server
     *
     * @see gotReply, parseJson
     */
    virtual Status parseReply(QNetworkReply* reply);

    /**
     * Processes the JSON document received from the Matrix server.
     * By default returns successful status without analysing the JSON.
     *
     * @param json valid JSON document received from the server
     *
     * @see parseReply
     */
    virtual Status parseJson(const QJsonDocument&);

    /**
     * Processes the reply in case of unsuccessful HTTP code.
     * The body is already loaded from the reply object to errorJson.
     * @param reply the HTTP reply from the server
     * @param errorJson the JSON payload describing the error
     */
    virtual Status parseError(QNetworkReply* reply,
                              const QJsonObject& errorJson);

    void setStatus(Status s);
    void setStatus(int code, QString message);

    // Q_DECLARE_LOGGING_CATEGORY return different function types
    // in different versions
    using LoggingCategory = decltype(JOBS)*;
    void setLoggingCategory(LoggingCategory lcf);

    // Job objects should only be deleted via QObject::deleteLater
    ~BaseJob() override;

protected slots:
    void timeout();

private slots:
    void sendRequest(bool inBackground);
    void checkReply();
    void gotReply();

private:
    void stop();
    void finishJob();

    class Private;
    QScopedPointer<Private> d;
};

inline bool isJobRunning(BaseJob* job)
{
    return job && job->error() == BaseJob::Pending;
}
} // namespace QMatrixClient

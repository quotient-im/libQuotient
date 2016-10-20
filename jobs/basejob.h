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

#ifndef QMATRIXCLIENT_BASEJOB_H
#define QMATRIXCLIENT_BASEJOB_H

#include <QtCore/QObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrlQuery>
#include <QtCore/QScopedPointer>

class QNetworkReply;
class QSslError;

namespace QMatrixClient
{
    class ConnectionData;

    enum class JobHttpType { GetJob, PutJob, PostJob, DeleteJob };

    class BaseJob: public QObject
    {
            Q_OBJECT
        public:
            /* Just in case, the values are compatible with KJob
             * (which BaseJob used to inherit from). */
            enum StatusCode { NoError = 0 // To be compatible with Qt conventions
                , Success = 0
                , ErrorLevel = 100 // Errors have codes starting from this
                , NetworkError = 100
                , JsonParseError
                , TimeoutError
                , ContentAccessError
                , UserDefinedError = 200
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
                    Query(const QList< QPair<QString, QString> >& l)
                    {
                        setQueryItems(l);
                    }
            };
            /**
             * A simple wrapper around QJsonObject that represents a JSON data
             * section of an HTTP request to a Matrix server. Facilitates
             * creation from a list of key-value string pairs and dumping of
             * a resulting JSON to a QByteArray.
             */
            class Data : public QJsonObject
            {
                public:
                    Data() = default;
                    Data(const QList< QPair<QString, QString> >& l)
                    {
                        for (auto i: l)
                            insert(i.first, i.second);
                    }
                    QByteArray serialize() const
                    {
                        return QJsonDocument(*this).toJson();
                    }
            };

            /**
             * This structure stores the status of a server call job. The status consists
             * of a code, that is described (but not delimited) by the respective enum,
             * and a freeform message.
             *
             * To extend the list of error codes, define an (anonymous) enum
             * along the lines of StatusCode, with additional values
             * starting at UserDefinedError
             */
            class Status
            {
                public:
                    Status(StatusCode c) : code(c) { }
                    Status(int c, QString m) : code(c), message(m) { }

                    bool good() const { return code < ErrorLevel; }

                    int code;
                    QString message;
            };

        public:
            BaseJob(ConnectionData* connection, JobHttpType type, QString name,
                    QString endpoint, Query query = Query(), Data data = Data(),
                    bool needsToken = true);
            virtual ~BaseJob();

            void start();

            /**
             * Abandons the result of this job, arrived or unarrived.
             *
             * This aborts waiting for a reply from the server (if there was
             * any pending) and deletes the job object. It is always done quietly
             * (as opposed to KJob::kill() that can trigger emitting the result).
             */
            void abandon();

            Status status() const;
            int error() const;
            virtual QString errorString() const;

        signals:
            /**
             * Emitted when the job is finished, in any case. It is used to notify
             * observers that the job is terminated and that progress can be hidden.
             *
             * This should not be emitted directly by subclasses;
             * use emitResult() instead.
             *
             * In general, to be notified of a job's completion, client code
             * should connect to success() and failure()
             * rather than finished(), so that kill() is indeed quiet.
             * However if you store a list of jobs and they might get killed
             * silently, then you must connect to this instead of result(),
             * to avoid dangling pointers in your list.
             *
             * @param job the job that emitted this signal
             * @internal
             *
             * @see success, failure
             */
            void finished(BaseJob* job);

            /**
             * Emitted when the job is finished (except when killed).
             *
             * Use error to know if the job was finished with error.
             *
             * @param job the job that emitted this signal
             *
             * @see success, failure
             */
            void result(BaseJob* job);

            /**
             * Emitted together with result() but only if there's no error.
             */
            void success(BaseJob*);

            /**
             * Emitted together with result() if there's an error.
             * Same as result(), this won't be emitted in case of kill().
             */
            void failure(BaseJob*);

        protected:
            ConnectionData* connection() const;

            const QUrlQuery& query() const;
            void setRequestQuery(const QUrlQuery& query);
            const Data& requestData() const;
            void setRequestData(const Data& data);

            /**
             * Used by gotReply() slot to check the received reply for general
             * issues such as network errors or access denial.
             * Returning anything except NoError/Success prevents
             * further parseReply()/parseJson() invocation.
             *
             * @param reply the reply received from the server
             * @return the result of checking the reply
             *
             * @see gotReply
             */
            virtual Status checkReply(QNetworkReply* reply) const;

            /**
             * Processes the reply. By default, parses the reply into
             * a QJsonDocument and calls parseJson() if it's a valid JSON.
             *
             * @param data raw contents of a HTTP reply from the server (without headers)
             *
             * @see gotReply, parseJson
             */
            virtual Status parseReply(QByteArray data);

            /**
             * Processes the JSON document received from the Matrix server.
             * By default returns succesful status without analysing the JSON.
             *
             * @param json valid JSON document received from the server
             *
             * @see parseReply
             */
            virtual Status parseJson(const QJsonDocument&);
            
            void setStatus(Status s);
            void setStatus(int code, QString message);

        protected slots:
            void timeout();
            void sslErrors(const QList<QSslError>& errors);

        private slots:
            void gotReply();

        private:
            void finishJob(bool emitResult);

            class Private;
            QScopedPointer<Private> d;
    };
}

#endif // QMATRIXCLIENT_BASEJOB_H

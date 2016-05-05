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

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkReply>

namespace QMatrixClient
{
    class ConnectionData;

    enum class JobHttpType { GetJob, PutJob, PostJob };
    
    class BaseJob: public QObject
    {
            Q_OBJECT
        public:
            /* Just in case, the values are compatible with KJob
             * (which BaseJob used to inherit from). */
            enum ErrorCode { NoError = 0, NetworkError = 100,
                             JsonParseError, TimeoutError, ContentAccessError,
                             UserDefinedError = 512 };

            BaseJob(ConnectionData* connection, JobHttpType type,
                    QString name, bool needsToken=true);
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

            // to implement
            virtual QString apiPath() const = 0;
            virtual QUrlQuery query() const;
            virtual QJsonObject data() const;
            virtual void parseJson(const QJsonDocument& data);
            
            /**
             * Sets the error code.
             *
             * It should be called when an error is encountered in the job,
             * just before calling emitResult(). Normally you might want to
             * use fail() instead - it sets error code, error text, makes sure
             * the job has finished and invokes emitResult after that.
             *
             * To extend the list of error codes, define an (anonymous) enum
             * with additional values starting at BaseJob::UserDefinedError
             *
             * @param errorCode the error code
             * @see emitResult(), fail()
             */
            void setError(int errorCode);
            void setErrorText(QString errorText);

            /**
             * Utility function to emit the result signal, and suicide this job.
             * It first notifies the observers to hide the progress for this job using
             * the finished() signal.
             *
             * @note: Deletes this job using deleteLater().
             *
             * @see result()
             * @see finished()
             */
            void emitResult();
            void fail( int errorCode, QString errorString );
            QNetworkReply* networkReply() const;

            
        protected slots:
            virtual void gotReply();
            void timeout();
            void sslErrors(const QList<QSslError>& errors);

        private:
            void finishJob(bool emitResult);

            class Private;
            Private* d;
    };
}

#endif // QMATRIXCLIENT_BASEJOB_H

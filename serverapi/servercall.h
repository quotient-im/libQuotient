/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
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

#pragma once

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include "servercallsetup.h"

class QNetworkReply;
class QSslError;
class QByteArray;

namespace QMatrixClient
{
    class ConnectionData;
//    class CallStatus;
//    class RequestParams;

namespace ServerApi
{
    class CallBase : public QObject
    {
            Q_OBJECT

        protected:
            // This class is for inheritance only (as of now, at least)
            CallBase(ConnectionData* data, QString name,
                           const RequestParams& params);
            // Deletion will be done from within the object; don't delete explicitly
            virtual ~CallBase();

        public:
            /**
             * Abandons the server call.
             *
             * This stops waiting for a reply from the server and arranges
             * deletion of the server call object.
             */
            void abandon();

            /**
             * Returns the current status of the server call. Note that
             * the status before the reply has arrived is PendingResult, not
             * NoError - use CallStatus::good() on the returned structure
             * to check whether the server call is in a good shape.
             *
             * @return the current status of the server call.
             */
            CallStatus status() const;

            /**
             * Provides access to the data from the server in their original
             * form, as arrived from the network.
             *
             */
            const QByteArray& rawData() const;

        signals:
            /**
             * Emitted when a call has finished, succesfully or unsuccesfully
             * (except when killed). Use status() to know if the call finished
             * with error. Alternatively, connect to success() and/or failure()
             * signals that only get emitted for successful or failed calls,
             * respectively.
             * There's no guarantee to emit resultReady() in a particular order
             * with success() and failure(); the current implementation emits
             * resultReady() first but this can change at any time. Connecting
             * to resultReady() and any of success() or failure() at the same time
             * is generally a bad idea; in such cases connecting to resultReady()
             * and using a check like 'if (status().good())' in the handler
             * should be sufficient to do everything.
             *
             * @note: The name is borrowed from QFutureWatcher that has
             * resultReadyAt() signal.
             *
             * @see success, failure, ServerCall<>::onAnyResult
             */
            void resultReady();

            /**
             * Emitted together with result() but only if there's no error.
             */
            void success();

            /**
             * Emitted together with result() if there's an error.
             * Same as result(), this won't be emitted in case of kill().
             */
            void failure();

        protected:
            /**
             * Sets the call to the specified status. setStatus() doesn't
             * emit signals with the status; to do that, use finish() after
             * setting the final call status.
             *
             * @see finish, CallStatus
             */
            void setStatus(CallStatus cs);
            /**
             * Same as setStatus(CallStatus), for cases when there's only a bare
             * code in the status. Clears the message part of the status.
             *
             * @param code a status code, as defined in CallStatus or custom extensions
             *
             * @see CallStatus
             */
            void setStatus(int code);

            /**
             * Provides access to a (possibly pending) network reply object.
             * The memory allocated for this object is managed inside ServerCallBase,
             * and shouldn't be freed externally.
             */
            QNetworkReply* reply() const;

            /**
             * Utility function to emit (if requested) signals depending on
             * the reply state and self-delete the server call object.
             *
             * @note: Deletes the object using deleteLater()
             *
             * @param emitResult if true, all necessary signals will be emitted
             * according to the current status of the server call. If false,
             * the server call completes and self-deletes silently.
             *
             * @see resultReady()
             */
            void finish(bool emitResult);

        private slots:
            void gotReply();
            void timeout();
            void sslErrors(const QList<QSslError>& errors);

        protected:
            virtual CallStatus makeResult(const QByteArray& bytes) = 0;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    template <class SetupT>
    class Call : public CallBase
    {
        public:
            Call(ConnectionData* c, SetupT&& s)
                : CallBase(c, s.name(), s.requestParams())
                , setup(std::forward<SetupT>(s))
            { }

            /**
             * After the server call completes, calling this will get
             * a reference to the object with server call results. Use it
             * if you have only a pointer to the ServerCall<> object at hand.
             * Alternatively, use callback adaptors (onAnyResult, onSuccess,
             * onFailure) to subscribe to resultReady(), success() and failure()
             * signals respectively; the callback adaptors will do the work
             * of storing the ServerCall<> pointer and supplying your handler
             * with the actual results object.
             *
             * @see onAnyResult, onSuccess, onFailure
             */
            const SetupT& results() const { return setup; }

            /**
             * Connects the supplied handler to the resultReady() signal.
             *
             * @param handler a function or a function object that accepts
             * a single parameter of type SetupT.
             *
             * @see ServerCallBase::resultReady
             */
            template <typename HandlerT>
            void onAnyResult(HandlerT handler)
            {
                connect(this, &CallBase::resultReady,
                        [=]() { handler(results()); });
            }

            /**
             * Connects the supplied handler to the success() signal.
             *
             * @param handler a function or a function object that accepts
             * a single parameter of type SetupT.
             *
             * @see ServerCallBase::success
             */
            template <typename HandlerT>
            void onSuccess(HandlerT handler) const
            {
                connect(this, &CallBase::success,
                        [=]() { handler(results()); });
            }

            /**
             * Connects the supplied handler to the failure() signal.
             *
             * @param handler a function or a function object that accepts
             * a single parameter of type SetupT.
             *
             * @see ServerCallBase::failure
             */
            template <typename HandlerT>
            void onFailure(HandlerT handler)
            {
                connect(this, &CallBase::failure,
                        [=]() { handler(status()); });
            }

        protected:
            virtual CallStatus makeResult(const QByteArray& bytes) override
            {
                const Result<typename SetupT::envelope_type> r = setup.preprocess(bytes);
                if (r.status().good())
                    return setup.fillResult(r);
                else
                    return r.status();
            }

        private:
            SetupT setup;
    };
}
}

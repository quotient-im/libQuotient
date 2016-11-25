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

namespace ServerApi
{
    class Call : public QObject
    {
            Q_OBJECT
        protected:
            // Deletion will be done from within the object; don't delete explicitly
            virtual ~Call();

        public:
            Call(ConnectionData* data, const RequestConfig& params);

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
             * @see success, failure, PendingCall::onAnyResult
             */
            void resultReady();

            /**
             * Emitted together with result() but only if there's no error.
             *
             * @see PendingCall::onSuccess
             */
            void success();

            /**
             * Emitted together with result() if there's an error.
             * Same as result(), this won't be emitted in case of abandon().
             *
             * @see PendingCall::onFailure
             */
            void failure(Status);

        protected:
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
            void finish(Status status, bool emitResult);

        private slots:
            virtual void gotReply() = 0;
            void timeout();
            void sslErrors(const QList<QSslError>& errors);

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    namespace impl
    {
        template <class T>
        struct Wrap
        {
            using type = Result<T>;
        };

        template <class AlreadyWrappedT>
        struct Wrap< Result<AlreadyWrappedT> >
        {
            using type = Result<AlreadyWrappedT>;
        };

        template <>
        struct Wrap<Status>
        {
            using type = Result<void>;
        };

        template <typename T>
        using wrap_t = typename Wrap<T>::type;

        template <class CallConfigT>
        inline auto fillResult(QNetworkReply* reply, const CallConfigT& config)
            -> decltype(config.parseReply(QJsonObject()))
        {
            const Result<QJsonObject> src = config.readAsJson(reply);
            if (src.status().good())
                return config.parseReply(src.get());
            else
                return src.status();
        }

        template <class CallConfigT>
        inline auto fillResult(QNetworkReply* reply, const CallConfigT& config)
            -> decltype(config.parseReply(reply))
        {
            return config.parseReply(reply);
        }
    }

    template <typename ConfigT>
    class PendingCall : public Call
    {
        private:
            const ConfigT config;

            // result_type is basically whatever comes from ConfigT::parseReply(),
            // wrapped into Result<>
            using result_type =
                impl::wrap_t<decltype(impl::fillResult(nullptr, config))>;
            result_type promise;

        public:
            using result_cref_type = typename result_type::cref_type;

            PendingCall(ConnectionData* cd, const ConfigT& cfg)
                : Call(cd, cfg), config(cfg), promise(PendingResult)
            { }

            /**
             * Aborts the server call.
             *
             * This stops waiting for a reply from the server and arranges
             * deletion of the server call object.
             */
            void abandon() { finish(status(), false); }

            /**
             * Get the server call result, after the server call completes
             *
             * After the server call completes, use this method to get an
             * object with server call results. You'll most likely need it in a
             * slot connected to the success() signal. Alternatively, use
             * onSuccess() callback adaptor that provides a shorter syntax for
             * connecting to success().
             *
             * @return a const reference to an object with server call results,
             * once the server call completes; a default-constructed object
             * before server call completion
             *
             * @see onSuccess
             */
            result_cref_type result() const { return promise.get(); }

            /**
             * Returns the current status of the server call. Note that
             * the status before the reply has arrived is PendingResult, not
             * NoError - use Status::good() on the returned structure
             * to check whether the server call is in a good shape.
             *
             * @return the current status of the server call.
             */
            Status status() const { return promise.status(); }

            /**
             * Connects the supplied handler to the resultReady() signal.
             *
             * @param handler a function or a function object that accepts
             * a single parameter of type SetupT.
             *
             * @see Call::resultReady
             */
            template <typename HandlerT>
            void onAnyResult(HandlerT handler)
            {
                connect(this, &Call::resultReady, [=]() { handler(promise); });
            }

            /**
             * Connects the supplied handler to the success() signal.
             *
             * @param handler a function or a function object that accepts
             * a single parameter of type SetupT.
             *
             * @see Call::success
             */
            template <typename HandlerT>
            void onSuccess(HandlerT handler) const
            {
                connect(this, &Call::success, [=]() { promise.passTo(handler); });
            }

            /**
             * Connects the supplied handler to the failure() signal.
             *
             * @param handler a function or a function object that accepts
             * a single parameter of type SetupT.
             *
             * @see Call::failure
             */
            template <typename HandlerT>
            void onFailure(HandlerT handler)
            {
                connect(this, &Call::failure, [=]() { handler(status()); });
            }

        protected:
            /* Deletion will be done from within the object; don't delete explicitly. */
            virtual ~PendingCall() { }

            virtual void gotReply()
            {
                // Only the status part is filled in m_result from checkReply().
                promise = config.checkReply(reply());
                if (promise.status().good())
                    promise = impl::fillResult(reply(), config);

                finish(promise.status(), true);
            }

    };
}
}

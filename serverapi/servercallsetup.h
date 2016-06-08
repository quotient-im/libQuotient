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

#include <QtCore/QUrlQuery>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>

#include <functional>

class QNetworkReply;

namespace QMatrixClient
{
    namespace ServerApi
    {
        class Query : public QUrlQuery
        {
            public:
                using QUrlQuery::QUrlQuery;
                Query() = default;
                explicit Query(QList<QPair<QString, QString> > l)
                {
                    setQueryItems(l);
                }
        };
        class Data : public QJsonObject
        {
            public:
                Data() = default;
                explicit Data(QList<QPair<QString, QString> > l)
                {
                    for (auto i: l)
                        insert(i.first, i.second);
                }
        };
        enum class HttpVerb { Get, Put, Post };

        class RequestConfig
        {
            public:
                RequestConfig(QString name, HttpVerb type, QString endpoint,
                        Query query = Query(), Data data = Data(), bool needsToken = true)
                    : m_name(name), m_type(type), m_endpoint(endpoint)
                    , m_query(query), m_data(data)
                    , m_needsToken(needsToken)
                { }

                QString name() const { return m_name; }
                HttpVerb type() const { return m_type; }
                QString apiPath() const { return m_endpoint; }
                QUrlQuery query() const { return m_query; }
                QByteArray data() const { return QJsonDocument(m_data).toJson(); }
                bool needsToken() const { return m_needsToken; }

            private:
                QString m_name;
                HttpVerb m_type;
                QString m_endpoint;
                Query m_query;
                Data m_data;
                bool m_needsToken;
        };

        enum StatusCode { NoError = 0 // To be compatible with Qt conventions
            , InfoLevel = 0 // Information codes below
            , Success = 0
            , PendingResult = 1
            , WarningLevel = 50 // Warnings below
            , ErrorLevel = 100 // Errors below
            , NetworkError = 100
            , JsonParseError
            , TimeoutError
            , ContentAccessError
            , UserDefinedError = 200
        };

        /**
         * This structure stores status of a server call. The status consists
         * of a code, that is described (but not delimited) by the respective enum,
         * and a freeform message.
         *
         * To extend the list of error codes, define an (anonymous) enum
         * with additional values starting at CallStatus::UserDefinedError
         *
         * @see Call, CallSetup
         */
        class Status
        {
            public:
                Status(StatusCode c) : code(c) { }
                Status(int c, QString m) : code(c), message(m) { }

                bool good() const { return code < WarningLevel; }

                int code;
                QString message;
        };

        /*
         * This class has common definitions for Result<> and its <void> specialization.
         * Do not use directly, use Result<void> instead.
         */
        class ResultBase
        {
            public:
                ResultBase(const Status& cs) : m_status(cs) { }
                ResultBase(int code, QString message) : m_status(code, message) { }

                Status status() const { return m_status; }
                explicit operator Status () const { return status(); }

            protected:
                Status m_status;
        };

        /**
         * This class encapsulates a return value from a result handling function
         * (either an adaptor, or makeResult) packed together with CallStatus.
         * This allows to return error codes and messages without using exceptions.
         * A notable case is Result<void>, which represents a result with no value,
         * only with a status but provides respective interface.
         */
        template <typename T>
        class Result : public ResultBase
        {
            public:
                using cref_type = const T&;

                using ResultBase::ResultBase;

                Result(const T& value, const Status& cs = Success)
                    : ResultBase(cs), m_value(value)
                { }
                Result(T&& value, const Status& cs = Success)
                    : ResultBase(cs), m_value(std::move(value))
                { }

                cref_type get() const { return m_value; }
                explicit operator cref_type () const { return get(); }

                template <typename HandlerT>
                void passTo(HandlerT handler) const { handler(get()); }

            private:
                T m_value;
        };

        template <>
        class Result<void> : public ResultBase
        {
            public:
                using cref_type = void;

                using ResultBase::ResultBase;

                template <typename HandlerT>
                void passTo(HandlerT handler) const { handler(); }
        };

        /**
         * @brief A base class for a call configuration without result data
         *
         * Derive a call configuration from this class if the call only returns
         * the status, without additional data. Reimplement checkReply() if you
         * need to customise the reply checking. This class also serves as a
         * base for CallConfig, the general-purpose call configuration class
         * which overrides makeResult() with a full-fledged result-making strategy.
         */
        class CallConfigBase: public RequestConfig
        {
            public:
                using RequestConfig::RequestConfig;
                using result_type = Result<void>;

                Status checkReply(const QNetworkReply* reply) const;

                template <typename ActualConfigT>
                static result_type makeResult(QNetworkReply* reply,
                                              const ActualConfigT& config)
                {
                    return config.checkReply(reply);
                }
        };

        class FromByteArray
        {
            public:
                using reply_raw_type = QByteArray;
                static QByteArray load(QNetworkReply* reply);
        };

        class FromJsonObject
        {
            public:
                using reply_raw_type = QJsonObject;
                static Result<QJsonObject> load(QNetworkReply* reply);
        };

        /**
         * @brief A base class for a call configuration that requires parsing
         * the reply payload
         *
         * This is a base class for call configurations that need to process
         * the reply body - most configurations fall under this category.
         *
         * Derived classes should:
         * - Specify as template parameters:
         *   - the actual result type - the one that will be exposed by the call
         *     to the outside world;
         *   - optionally, the reply loader; by default, a QJsonObject reply
         *     loader is used but you may want to use FromByteArray to parse
         *     the result directly from the reply payload;
         * - Provide a constructor or constructors that will initialize
         *   RequestConfig inside the base class;
         * - Provide a function or a function object with the name parseReply
         *   that can accept the type produced by the reply loader; the default
         *   implementation blindly returns Success without analysing the reply.
         *
         * The overall result-building strategy is laid out in makeResult() -
         * you can override this as well if you need a more sophisticated or
         * just different logic. You should consider deriving from CallConfigBase
         * in that case, though.
         *
         * @see RequestConfig, Call, PendingCall
         */
        template <typename ResultT, typename ReplyLoaderT = FromJsonObject>
        class CallConfig : public CallConfigBase
        {
            public:
                using CallConfigBase::CallConfigBase; // see RequestConfig
                using result_type = Result<ResultT>;
                using reply_raw_type = typename ReplyLoaderT::reply_raw_type;

                /**
                 * @brief callback for reply parsing after preparation
                 *
                 * Override this in your call config class to actually process
                 * the reply.
                 */
                Result<ResultT> parseReply(const reply_raw_type&) const
                {
                    return {};
                }

                template <typename ActualConfigT>
                static result_type makeResult(QNetworkReply* reply,
                                              const ActualConfigT& config)
                {
                    const Status cs = config.checkReply(reply);
                    if (!cs.good())
                        return cs;

                    const Result<reply_raw_type> src = ReplyLoaderT::load(reply);
                    if (src.status().good())
                        return config.parseReply(src.get());
                    else
                        return src.status();
                }
        };
    }
}

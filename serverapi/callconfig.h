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

#include <QtCore/QJsonObject>
#include <QtCore/QString>

#include "../jobs/requestconfig.h"

class QNetworkReply;

namespace QMatrixClient
{
    namespace ServerApi
    {
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

        /**
         * This class encapsulates a return value packed together with Status.
         * This allows to return error codes and messages without using exceptions.
         * A notable case is Result<void>, which represents a result with no value,
         * only with a status; but provides similar interface.
         */
        template <typename T>
        class Result
        {
            public:
                using cref_type = const T&;

                Result(const Status& cs) : m_status(cs) { }
                Result(int code, QString message) : m_status(code, message) { }
                Result(const T& value, const Status& cs = Success)
                    : m_status(cs), m_value(value)
                { }
                Result(T&& value, const Status& cs = Success)
                    : m_status(cs), m_value(std::move(value))
                { }

                Status status() const { return m_status; }
                cref_type get() const { return m_value; }

                template <typename HandlerT>
                void passTo(HandlerT handler) const { handler(get()); }

            private:
                Status m_status;
                T m_value;
        };

        template <>
        class Result<void>
        {
            public:
                using cref_type = void;

                Result(const Status& cs) : m_status(cs) { }
                Result(int code, QString message) : m_status(code, message) { }

                Status status() const { return m_status; }

                template <typename HandlerT>
                void passTo(HandlerT handler) const { handler(); }

            private:
                Status m_status;
        };

        /**
         * @brief A base class for a call configuration
         *
         * Derive from this class to make a call configuration that can be used
         * with PendingCall<>. Derived classes should:
         * - Provide a constructor or constructors that will initialize
         *   RequestConfig fields inside the base class;
         * - (optionally) Provide a function or a function object with the same
         *   signature as checkReply() below to override the network reply
         *   preliminary checking.
         * - Provide a function or a function object with the name
         *   parseReply that accepts either QNetworkReply* or QJsonObject
         *   and returns a data structure (optionally enclosed into Result<>)
         *   parsed from the reply or from the respective JSON. Only one overload
         *   should exist; having both QNetworkReply* and QJsonObject versions
         *   will cause a compilation error.
         *
         * @see RequestConfig, Call, PendingCall
         */
        class CallConfig : public RequestConfig
        {
            public:
                using RequestConfig::RequestConfig;

                Status checkReply(const QNetworkReply* reply) const;
                Result<QJsonObject> readAsJson(QNetworkReply* reply) const;

                // Implement one of the following:
//                <return-type> parseReply(QNetworkReply*) const;
//                <return-type> parseReply(const QJsonObject&) const;
        };

        /**
         * @brief CallConfig for calls with empty/ignored reply contents
         *
         * This is a facility class for cases when do not need/want to
         * analyze the reply from the server, except a plain check for
         * successful completion ('200').
         */
        class SimpleCallConfig : public CallConfig
        {
            public:
                using CallConfig::CallConfig;

                Status parseReply(QNetworkReply*) const { return Success; }
        };
    }
}

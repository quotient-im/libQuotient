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

#include "requestparams.h"

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QDebug>

namespace QMatrixClient
{
namespace ServerApi
{
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
    class CallStatus
    {
        public:
            enum Code { NoError = 0 // To be compatible with Qt conventions
                , InfoLevel = 0 // Information codes below
                , Success = 0
                , PendingResult = 1
                , WarningLevel = 50 // Warnings below
                , ErrorLevel = 100 // Errors below
                , NetworkError = 100
                , JsonParseError
                , TimeoutError
                , UserDefinedError = 200
            };

            explicit CallStatus(int c = PendingResult) : code(c) { }
            CallStatus(int c, QString m) : code(c), message(m) { }

            void set(int c) { code = c; message.clear(); }
            void set(int c, QString m) { code = c; message = m; }

            bool good() const { return code < WarningLevel; }

            int code;
            QString message;
    };
    inline QDebug operator<< (QDebug dbg, CallStatus cs) {
        return dbg << "CallStatus(" << cs.code << "," << cs.message << ")";
    }
    const CallStatus CallSuccess { CallStatus::Success };

    /**
     * This class encapsulates a return value from a result handling function
     * (either an adaptor, or makeResult) packed together with CallStatus.
     * This allows to return error codes and messages without using exceptions.
     * Result<void> has a special (but obvious) meaning: it stores no return
     * value, only a CallStatus.
     */
    template <typename T>
    class Result
    {
        public:
            Result(const T& value)
                : m_data(value), m_status(CallStatus::Success)
            { }

            Result(const CallStatus& cs) : m_status(cs) { }
            Result(int code, QString message) : m_status(code, message) { }

            Result& operator=(const T& value)
            {
                return *this = Result(value);
            }

            const T& get() const { return m_data; }
            operator const T& () const { return get(); }

            CallStatus status() const { return m_status; }
            operator CallStatus () const { return status(); }

        private:
            T m_data;
            CallStatus m_status;
    };

    template <>
    class Result<void>
    {
        public:
            Result(const CallStatus& cs) : m_status(cs) { }
            Result(int code, QString message) : m_status(code, message) { }

            CallStatus status() const { return m_status; }
            operator CallStatus () const { return status(); }

        private:
            CallStatus m_status;
    };

    // The following two types define two stock result adaptors to be used.
    // If you write a custom result adaptor, you should:
    // - define output_type that specifies the output type of your adaptor;
    // - provide a function or a function object equivalent to:
    //       output_type preprocess(const QByteArray&)
    //   The return value type should be output_type or Result<output_type>, or
    //   implicitly castable to either type.

    /**
     * This no-op adaptor passes the reply contents without any actions.
     */
    struct GetBytes
    {
        using output_type = QByteArray;
        static const QByteArray& preprocess(const QByteArray& bytes)
        {
            return bytes;
        }
    };

    /**
     * This adaptor checks the reply contents to be a valid JSON document
     * that has a single object on the top level, parses the document and
     * returns the resulting structure wrapped into a QJsonObject.
     */
    struct GetJson
    {
        using output_type = QJsonObject;
        static Result<QJsonObject> preprocess(QByteArray bytes);
    };

    /**
     * This is a base class for setups that describe specific server calls
     * (to specific endpoints, in particular). Derived classes should pick
     * an appropriate ResultAdaptorT and implement two things:
     * - Constructor that initializes CallSetup<> - in particular,
     *   supplies a valid RequestParams object.
     * - a function or a function object fillResult that can accept
     *   preprocessed_type, which is the type produced by the chosen preprocessor
     *   type (QJsonObject by default).
     *
     * @see RequestParams, Call
     */
    template <typename ResultAdaptorT = GetJson>
    class CallData : public ResultAdaptorT
    {
        public:
            using envelope_type = typename ResultAdaptorT::output_type;

            CallData(QString name, RequestParams rp)
                : m_name(name), m_reqParams(rp)
            { }

            QString name() const { return m_name; }
            const RequestParams& requestParams() const { return m_reqParams; }


            CallStatus fillResult(envelope_type)
            {
                return CallSuccess;
            }

        private:
            QString m_name;
            RequestParams m_reqParams;
    };
}
}

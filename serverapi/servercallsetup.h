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

namespace QMatrixClient
{
    /**
     * This structure stores status of a server call. The status consists
     * of a code, that is described (but not delimited) by the respective enum,
     * and a freeform message.
     *
     * To extend the list of error codes, define an (anonymous) enum
     * with additional values starting at CallStatus::UserDefinedError
     *
     * @see ServerCall, ServerCallSetup
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

    // A supplementary base class for ServerCallSetup<> to pass the shared
    // status to ServerCallBase. Do not derive from it directly, use ServerCallSetup<>.
    class ServerCallSetupBase
    {
        public:
            ServerCallSetupBase(QString name, RequestParams rp)
                : m_name(name), m_reqParams(rp)
            { }

            QString name() const { return m_name; }
            const RequestParams& requestParams() const { return m_reqParams; }

            const CallStatus status() const { return m_status; }

        protected:
            void setStatus(int code) { setStatus(CallStatus(code)); }
            void setStatus(int c, QString m) { setStatus(CallStatus(c,m)); }
            void setStatus(CallStatus cs) { m_status = cs; }

        private:
            QString m_name;
            RequestParams m_reqParams;
            CallStatus m_status;
    };

    // The following two types define two stock result preprocessors to be used.
    // If you write a custom preprocessor, you should:
    // - provide a constructor with the same signature as the one of ServerCallSetupBase
    //   ('using', as below, is the easiest way)
    // - define preprocessed_type that specifies the output type of your preprocessor;
    // - provide a function or a function object equivalent to:
    //       preprocessed_type preprocess(const QByteArray&)
    //   The return value type can be implicitly castable to preprocessed_type

    /**
     * This no-op adaptor passes the reply contents without any actions.
     */
    class GetBytes : public ServerCallSetupBase
    {
        public:
            using ServerCallSetupBase::ServerCallSetupBase;

            using preprocessed_type = QByteArray;
            const QByteArray& preprocess(const QByteArray& bytes)
            {
                return bytes;
            }
    };

    /**
     * This adaptor checks the reply contents to be a valid JSON document
     * that has a single object on the top level, parses the document and
     * returns the resulting structure wrapped into a QJsonObject.
     */
    class GetJson : public ServerCallSetupBase
    {
        public:
            using ServerCallSetupBase::ServerCallSetupBase;

            using preprocessed_type = QJsonObject;
            QJsonObject preprocess(QByteArray bytes);
    };

    /**
     * This is a base class for setups that describe specific server calls
     * (to specific endpoints, in particular). Derived classes should pick
     * an appropriate ResultAdaptorT and implement two things:
     * - Constructor that initializes ServerCallSetup<> - in particular,
     *   supplies a valid RequestParams object.
     * - a function or a function object fillResult that can accept
     *   preprocessed_type, which is the type produced by the chosen preprocessor
     *   type (QJsonObject by default).
     *
     * @see RequestParams, ServerCall
     */
    template <class ResultAdaptorT = GetJson>
    class ServerCallSetup : public ResultAdaptorT
    {
        public:
            ServerCallSetup(QString name, RequestParams rp)
                : ResultAdaptorT(name, rp)
            { }

            using HttpType = RequestParams::HttpType;
            using Query = RequestParams::Query;
            using Data = RequestParams::Data;

            void fillResult(typename ResultAdaptorT::preprocessed_type)
            {
            }
    };
}

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

    // The following two types define two stock result adaptors to be used.
    // If you write a customer preprocessor, you should define:
    // - output_type type;
    // - a function (or a function object equivalent to it):
    //       output_type preprocess(const QByteArray&, CallStatus&)
    //   The return value can be of a type implicitly castable to output_type

    /**
     * This no-op adaptor passes the reply contents without any actions.
     */
    struct NoResultAdaptor
    {
        using output_type = QByteArray;
        static const QByteArray& preprocess(const QByteArray& bytes,
                                            CallStatus& /*unused*/)
        {
            return bytes;
        }
    };

    /**
     * This adaptor checks the reply contents to be a valid JSON document
     * that has a single object on the top level, parses the document and
     * returns the resulting structure wrapped into a QJsonObject.
     */
    struct JsonObjectResult
    {
        using output_type = QJsonObject;
        static QJsonObject preprocess(QByteArray bytes, CallStatus& status);
    };

    // A supplementary base class for ServerCallSetup<> to pass the shared
    // status to ServerCallBase. Not supposed to be derived from directly.
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

            friend class ServerCallBase;
            // Allow full access to status for the ServerCallBase class and
            // classes derived from ServerCallSetupBase.
            // in particular sending this reference further on.
            CallStatus* statusPtr() { return &m_status; }
        private:
            QString m_name;
            RequestParams m_reqParams;
            CallStatus m_status;
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
    template <class ResultAdaptorT = JsonObjectResult>
    class ServerCallSetup : public ServerCallSetupBase
    {
        public: // For use in derived classes initialization
            ServerCallSetup(QString name, RequestParams rp)
                : ServerCallSetupBase(name, rp)
            { }

            using HttpType = RequestParams::HttpType;
            using Query = RequestParams::Query;
            using Data = RequestParams::Data;

            using preprocessed_type = typename ResultAdaptorT::output_type;

            void fillResult(preprocessed_type)
            {
                Q_STATIC_ASSERT_X(true,
                    "When deriving from ServerCallSetup<>, "
                    "fillResult(preprocessed_type) should be redefined");
            }

        public:
            // If you want to do preprocessing inside the class derived from
            // ServerCallSetup<>, don't override this; just use NoResultAdaptor
            // and do preprocessing in fillResult() directly.
            preprocessed_type preprocess(const QByteArray& bytes)
            {
                return std::move(ResultAdaptorT::preprocess(bytes, *statusPtr()));
            }
    };

}

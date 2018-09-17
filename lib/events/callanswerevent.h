/******************************************************************************
 * Copyright (C) 2017 Marius Gripsgard <marius@ubports.com>
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

#include "roomevent.h"

namespace QMatrixClient
{
    class CallAnswerEvent: public RoomEvent
    {
    public:
        DEFINE_EVENT_TYPEID("m.call.answer", CallAnswerEvent)

        explicit CallAnswerEvent(const QJsonObject& obj);

        explicit CallAnswerEvent(const QString& callId, const int lifetime,
                                 const QString& sdp);
        explicit CallAnswerEvent(const QString& callId, const QString& sdp);

        bool isStateEvent() const override { return true; }

        const int lifetime() const { return _lifetime; }
        const QString& sdp() const { return _sdp; }
        const QString& callId() const { return _callId; }
        const int version() const { return _version; }

        QJsonObject toJson() const
        {
            QJsonObject answer;
            answer.insert("sdp", _sdp);
            answer.insert("type", QStringLiteral("answer"));

            QJsonObject obj;
            obj.insert("call_id", _callId);
            obj.insert("version", _version);
            if (_lifetime != NULL)
              obj.insert("lifetime", _lifetime);
            obj.insert("answer", answer);
            return obj;
        }

    private:
        int _lifetime;
        QJsonObject _answer;
        QString _sdp;
        QString _callId;
        int _version;
    };

    REGISTER_EVENT_TYPE(CallAnswerEvent)
    DEFINE_EVENTTYPE_ALIAS(CallAnswer, CallAnswerEvent)
} // namespace QMatrixClient

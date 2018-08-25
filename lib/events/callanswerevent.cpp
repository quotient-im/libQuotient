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

#include "callanswerevent.h"

#include "event.h"

#include "logging.h"

#include <QtCore/QJsonDocument>

/*
m.call.answer
{
    "age": 242352,
    "content": {
        "answer": {
            "sdp": "v=0\r\no=- 6584580628695956864 2 IN IP4 127.0.0.1[...]",
            "type": "answer"
        },
        "call_id": "12345",
        "lifetime": 60000,
        "version": 0
    },
    "event_id": "$WLGTSEFSEF:localhost",
    "origin_server_ts": 1431961217939,
    "room_id": "!Cuyf34gef24t:localhost",
    "sender": "@example:localhost",
    "type": "m.call.answer"
}
*/

using namespace QMatrixClient;

CallAnswerEvent::CallAnswerEvent(const QJsonObject& obj)
    : RoomEvent(CallAnswer, obj)
    , _lifetime(contentJson()["lifetime"].toInt())
    , _sdp(contentJson()["answer"].toObject()["sdp"].toString())
    , _callId(contentJson()["call_id"].toString())
    , _version(contentJson()["version"].toInt())
{
    qCDebug(EVENTS) << "Call Answer event";
}

CallAnswerEvent::CallAnswerEvent(const QString& callId, const int lifetime,
                                 const QString& sdp)
     : RoomEvent(CallAnswer)
{
    _version = 0;
    _callId = callId;
    _lifetime = lifetime;
    _sdp = sdp;
}

CallAnswerEvent::CallAnswerEvent(const QString& callId, const QString& sdp)
     : RoomEvent(CallAnswer)
{
    _version = 0;
    _callId = callId;
    _sdp = sdp;
}

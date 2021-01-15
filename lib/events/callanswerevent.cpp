/******************************************************************************
 * SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "callanswerevent.h"

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

using namespace Quotient;

CallAnswerEvent::CallAnswerEvent(const QJsonObject& obj)
    : CallEventBase(typeId(), obj)
{
    qCDebug(EVENTS) << "Call Answer event";
}

CallAnswerEvent::CallAnswerEvent(const QString& callId, const int lifetime,
                                 const QString& sdp)
    : CallEventBase(
        typeId(), matrixTypeId(), callId, 0,
        { { QStringLiteral("lifetime"), lifetime },
          { QStringLiteral("answer"),
            QJsonObject { { QStringLiteral("type"), QStringLiteral("answer") },
                          { QStringLiteral("sdp"), sdp } } } })
{}

CallAnswerEvent::CallAnswerEvent(const QString& callId, const QString& sdp)
    : CallEventBase(
        typeId(), matrixTypeId(), callId, 0,
        { { QStringLiteral("answer"),
            QJsonObject { { QStringLiteral("type"), QStringLiteral("answer") },
                          { QStringLiteral("sdp"), sdp } } } })
{}

// SPDX-FileCopyrightText: 2022 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "callevents.h"

#include "../logging_categories_p.h"

using namespace Quotient;

QJsonObject CallEvent::basicJson(const QString& matrixType,
                                 const QString& callId, int version, const QString& partyId,
                                 QJsonObject contentJson)
{
    contentJson.insert(QStringLiteral("call_id"), callId);
    contentJson.insert(QStringLiteral("version"), version);
    contentJson.insert(QStringLiteral("party_id"), partyId);
    return RoomEvent::basicJson(matrixType, contentJson);
}

CallEvent::CallEvent(const QJsonObject& json)
    : RoomEvent(json)
{
    if (callId().isEmpty())
        qCWarning(EVENTS) << id() << "is a call event with an empty call id";
}

/*
m.call.invite
{
    "age": 242352,
    "content": {
        "call_id": "12345",
        "party_id": "foo123",
        "lifetime": 60000,
        "offer": {
            "sdp": "v=0\r\no=- 6584580628695956864 2 IN IP4 127.0.0.1[...]",
            "type": "offer"
        },
        "version": "1"
    },
    "event_id": "$WLGTSEFSEF:localhost",
    "origin_server_ts": 1431961217939,
    "room_id": "!Cuyf34gef24t:localhost",
    "sender": "@example:localhost",
    "type": "m.call.invite"
}
*/

CallInviteEvent::CallInviteEvent(const QString& callId, const QString& partyId, int lifetime,
                                 const QString& sdp)
    : EventTemplate(
        callId, partyId,
        { { QStringLiteral("lifetime"), lifetime },
          { QStringLiteral("offer"),
            QJsonObject{ { QStringLiteral("type"), QStringLiteral("offer") },
                         { QStringLiteral("sdp"), sdp } } } })
{}

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
        "party_id": "foo123",
        "version": "1"
    },
    "event_id": "$WLGTSEFSEF:localhost",
    "origin_server_ts": 1431961217939,
    "room_id": "!Cuyf34gef24t:localhost",
    "sender": "@example:localhost",
    "type": "m.call.answer"
}
*/

CallAnswerEvent::CallAnswerEvent(const QString& callId, const QString& partyId, const QString& sdp)
    : EventTemplate(callId, partyId, { { QStringLiteral("answer"),
                            QJsonObject { { QStringLiteral("type"),
                                            QStringLiteral("answer") },
                                          { QStringLiteral("sdp"), sdp } } } })
{}

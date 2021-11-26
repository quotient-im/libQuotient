// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "callinviteevent.h"

/*
m.call.invite
{
    "age": 242352,
    "content": {
        "call_id": "12345",
        "lifetime": 60000,
        "offer": {
            "sdp": "v=0\r\no=- 6584580628695956864 2 IN IP4 127.0.0.1[...]",
            "type": "offer"
        },
        "version": 0
    },
    "event_id": "$WLGTSEFSEF:localhost",
    "origin_server_ts": 1431961217939,
    "room_id": "!Cuyf34gef24t:localhost",
    "sender": "@example:localhost",
    "type": "m.call.invite"
}
*/

using namespace Quotient;

CallInviteEvent::CallInviteEvent(const QJsonObject& obj)
    : CallEventBase(typeId(), obj)
{
    qCDebug(EVENTS) << "Call Invite event";
}

CallInviteEvent::CallInviteEvent(const QString& callId, const int lifetime,
                                 const QString& sdp)
    : CallEventBase(
        typeId(), matrixTypeId(), callId, lifetime,
        { { QStringLiteral("lifetime"), lifetime },
          { QStringLiteral("offer"),
            QJsonObject { { QStringLiteral("type"), QStringLiteral("offer") },
                          { QStringLiteral("sdp"), sdp } } } })
{}

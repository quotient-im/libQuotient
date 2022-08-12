// SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
// SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

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

CallAnswerEvent::CallAnswerEvent(const QString& callId, const QString& sdp)
    : EventTemplate(callId, { { QStringLiteral("answer"),
                            QJsonObject { { QStringLiteral("type"),
                                            QStringLiteral("answer") },
                                          { QStringLiteral("sdp"), sdp } } } })
{}

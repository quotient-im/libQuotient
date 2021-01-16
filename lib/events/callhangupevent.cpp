/******************************************************************************
 * SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
 * SPDX-FileCopyrightText: 2018 Josip Delic <delijati@googlemail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "callhangupevent.h"

/*
m.call.hangup
{
    "age": 242352,
    "content": {
        "call_id": "12345",
        "version": 0
    },
    "event_id": "$WLGTSEFSEF:localhost",
    "origin_server_ts": 1431961217939,
    "room_id": "!Cuyf34gef24t:localhost",
    "sender": "@example:localhost",
    "type": "m.call.hangup"
}
*/

using namespace Quotient;

CallHangupEvent::CallHangupEvent(const QJsonObject& obj)
    : CallEventBase(typeId(), obj)
{
    qCDebug(EVENTS) << "Call Hangup event";
}

CallHangupEvent::CallHangupEvent(const QString& callId)
    : CallEventBase(typeId(), matrixTypeId(), callId, 0)
{}

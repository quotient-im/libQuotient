/******************************************************************************
 * SPDX-FileCopyrightText: 2017 Marius Gripsgard <marius@ubports.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "callcandidatesevent.h"

/*
m.call.candidates
{
    "age": 242352,
    "content": {
        "call_id": "12345",
        "candidates": [
            {
                "candidate": "candidate:863018703 1 udp 2122260223 10.9.64.156
43670 typ host generation 0", "sdpMLineIndex": 0, "sdpMid": "audio"
            }
        ],
        "version": 0
    },
    "event_id": "$WLGTSEFSEF:localhost",
    "origin_server_ts": 1431961217939,
    "room_id": "!Cuyf34gef24t:localhost",
    "sender": "@example:localhost",
    "type": "m.call.candidates"
}
*/

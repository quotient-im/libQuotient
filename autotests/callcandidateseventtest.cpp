// SPDX-FileCopyrightText: 2020 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "events/callcandidatesevent.h"
#include "callcandidateseventtest.h"

void TestCallCandidatesEvent::fromJson()
{
    auto document = QJsonDocument::fromJson(R"({
        "age": 242352,
        "content": {
            "call_id": "12345",
            "candidates": [
                {
                    "candidate": "candidate:863018703 1 udp 2122260223 10.9.64.156 43670 typ host generation 0",
                    "sdpMLineIndex": 0,
                    "sdpMid": "audio"
                }
            ],
            "version": 0
        },
        "event_id": "$WLGTSEFSEF:localhost",
        "origin_server_ts": 1431961217939,
        "room_id": "!Cuyf34gef24t:localhost",
        "sender": "@example:localhost",
        "type": "m.call.candidates"
    })");

    QVERIFY(document.isObject());

    auto object = document.object();

    Quotient::CallCandidatesEvent callCandidatesEvent(object);

    QCOMPARE(callCandidatesEvent.version(), 0);
    QCOMPARE(callCandidatesEvent.callId(), "12345");
    QCOMPARE(callCandidatesEvent.candidates().count(), 1);

    const QJsonObject &candidate = callCandidatesEvent.candidates().at(0).toObject();
    QCOMPARE(candidate.value("sdpMid").toString(), "audio");
    QCOMPARE(candidate.value("sdpMLineIndex").toInt(), 0);
    QCOMPARE(candidate.value("candidate").toString(), "candidate:863018703 1 udp 2122260223 10.9.64.156 43670 typ host generation 0");
}

QTEST_MAIN(TestCallCandidatesEvent)
#include "callcandidateseventtest.moc"

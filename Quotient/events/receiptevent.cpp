// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
Example of a Receipt Event:
{
    "content": {
        "$1435641916114394fHBLK:matrix.org": {
            "m.read": {
                "@rikj:jki.re": {
                    "ts": 1436451550453
                }
            }
        }
    },
    "room_id": "!KpjVgQyZpzBwvMBsnT:matrix.org",
    "type": "m.receipt"
}
*/

#include "receiptevent.h"

#include <Quotient/logging.h>

using namespace Quotient;

// The library loads the event-ids-to-receipts JSON map into a vector because
// map lookups are not used and vectors are massively faster. Same goes for
// de-/serialization of ReceiptsForEvent::receipts.
// (XXX: would this be generally preferred across CS API JSON maps?..)
QJsonObject Quotient::toJson(const EventsWithReceipts& ewrs)
{
    QJsonObject json;
    for (const auto& e : ewrs) {
        QJsonObject receiptsJson;
        for (const auto& r : e.receipts)
            receiptsJson.insert(r.userId,
                                QJsonObject { { "ts"_ls, toJson(r.timestamp) } });
        json.insert(e.evtId, QJsonObject { { "m.read"_ls, receiptsJson } });
    }
    return json;
}

template<>
EventsWithReceipts Quotient::fromJson(const QJsonObject& json)
{
    EventsWithReceipts result;
    result.reserve(json.size());
    for (auto eventIt = json.begin(); eventIt != json.end(); ++eventIt) {
        if (eventIt.key().isEmpty()) {
            qCWarning(EPHEMERAL)
                << "ReceiptEvent has an empty event id, skipping";
            qCDebug(EPHEMERAL) << "ReceiptEvent content follows:\n" << json;
            continue;
        }
        const auto reads =
            eventIt.value().toObject().value("m.read"_ls).toObject();
        QVector<UserTimestamp> usersAtEvent;
        usersAtEvent.reserve(reads.size());
        for (auto userIt = reads.begin(); userIt != reads.end(); ++userIt) {
            const auto user = userIt.value().toObject();
            usersAtEvent.push_back(
                { userIt.key(), fromJson<QDateTime>(user["ts"_ls]) });
        }
        result.push_back({ eventIt.key(), std::move(usersAtEvent) });
    }
    return result;
}

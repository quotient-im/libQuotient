/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
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

#include "logging.h"

#include <QtCore/QJsonArray>

using namespace QMatrixClient;

ReceiptEvent::ReceiptEvent(const QJsonObject& obj)
    : Event(Type::Receipt, obj)
{
    Q_ASSERT(obj["type"].toString() == jsonType);

    const QJsonObject contents = contentJson();
    _eventsWithReceipts.reserve(static_cast<size_t>(contents.size()));
    for( auto eventIt = contents.begin(); eventIt != contents.end(); ++eventIt )
    {
        if (eventIt.key().isEmpty())
        {
            qCWarning(EPHEMERAL) << "ReceiptEvent has an empty event id, skipping";
            qCDebug(EPHEMERAL) << "ReceiptEvent content follows:\n" << contents;
            continue;
        }
        const QJsonObject reads = eventIt.value().toObject().value("m.read").toObject();
        std::vector<Receipt> receipts;
        receipts.reserve(static_cast<size_t>(reads.size()));
        for( auto userIt = reads.begin(); userIt != reads.end(); ++userIt )
        {
            const QJsonObject user = userIt.value().toObject();
            receipts.push_back({userIt.key(), toTimestamp(user["ts"])});
        }
        _eventsWithReceipts.push_back({eventIt.key(), receipts});
    }
    static const auto UnreadMsgsKey =
        QStringLiteral("x-qmatrixclient.unread_messages");
    if (contents.contains(UnreadMsgsKey))
        _unreadMessages = contents["x-qmatrixclient.unread_messages"].toBool();
    else
        _unreadMessages = obj["x-qmatrixclient.unread_messages"].toBool();
}


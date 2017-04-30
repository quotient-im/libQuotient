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
#include "util.h"

#include <QtCore/QJsonArray>
#include <QtCore/QDebug>

using namespace QMatrixClient;

class ReceiptEvent::Private
{
    public:
        EventsToReceipts eventsToReceipts;
};

ReceiptEvent::ReceiptEvent()
    : Event(EventType::Receipt)
    , d(new Private)
{
}

ReceiptEvent::~ReceiptEvent()
{
    delete d;
}

EventsToReceipts ReceiptEvent::events() const
{
    return d->eventsToReceipts;
}

ReceiptEvent* ReceiptEvent::fromJson(const QJsonObject& obj)
{
    ReceiptEvent* e = new ReceiptEvent();
    e->parseJson(obj);
    const QJsonObject contents = obj["content"].toObject();
    e->d->eventsToReceipts.reserve(contents.size());
    for( auto eventIt = contents.begin(); eventIt != contents.end(); ++eventIt )
    {
        if (eventIt.key().isEmpty())
        {
            qCWarning(EVENTS) << "ReceiptEvent has an empty event id, skipping";
            qCDebug(EVENTS) << "ReceiptEvent content follows:\n" << contents;
            continue;
        }
        const QJsonObject reads = eventIt.value().toObject().value("m.read").toObject();
        Receipts receipts; receipts.reserve(reads.size());
        for( auto userIt = reads.begin(); userIt != reads.end(); ++userIt )
        {
            const QJsonObject user = userIt.value().toObject();
            const auto time = QDateTime::fromMSecsSinceEpoch(
                        static_cast<qint64>(user["ts"].toDouble()), Qt::UTC );
            receipts.push_back({ userIt.key(), time });
        }
        e->d->eventsToReceipts.push_back({ eventIt.key(), receipts });
    }
    return e;
}

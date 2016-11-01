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

#include <QtCore/QJsonArray>
#include <QtCore/QDebug>

using namespace QMatrixClient;

class ReceiptEvent::Private
{
    public:
        QHash<QString, Receipts> eventToReceipts;
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

Receipts ReceiptEvent::receiptsForEvent(QString eventId) const
{
    return d->eventToReceipts.value(eventId);
}

QStringList ReceiptEvent::events() const
{
    return d->eventToReceipts.keys();
}

ReceiptEvent* ReceiptEvent::fromJson(const QJsonObject& obj)
{
    ReceiptEvent* e = new ReceiptEvent();
    e->parseJson(obj);
    const QJsonObject contents = obj["content"].toObject();
    e->d->eventToReceipts.reserve(contents.size());
    for( const QString& eventId: contents.keys() )
    {
        const QJsonObject reads = contents[eventId].toObject().value("m.read").toObject();
        Receipts receipts(reads.size());
        for( const QString& userId: reads.keys() )
        {
            const QJsonObject user = reads[userId].toObject();
            const QDateTime time = QDateTime::fromMSecsSinceEpoch( (quint64) user["ts"].toDouble(), Qt::UTC );
            receipts.push_back({ eventId, userId, time });
        }
        e->d->eventToReceipts.insert(eventId, receipts);
    }
    return e;
}

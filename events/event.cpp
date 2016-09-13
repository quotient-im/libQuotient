/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
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

#include "event.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include "../logging_util.h"
#include "roommessageevent.h"
#include "roomnameevent.h"
#include "roomaliasesevent.h"
#include "roomcanonicalaliasevent.h"
#include "roommemberevent.h"
#include "roomtopicevent.h"
#include "typingevent.h"
#include "receiptevent.h"
#include "unknownevent.h"

using namespace QMatrixClient;

class Event::Private
{
    public:
        EventType type;
        QString id;
        QDateTime timestamp;
        QString roomId;
        QString originalJson;
};

Event::Event(EventType type)
    : d(new Private)
{
    d->type = type;
}

Event::~Event()
{
    delete d;
}

EventType Event::type() const
{
    return d->type;
}

QString Event::id() const
{
    return d->id;
}

QDateTime Event::timestamp() const
{
    return d->timestamp;
}

QString Event::roomId() const
{
    return d->roomId;
}

QString Event::originalJson() const
{
    return d->originalJson;
}

template <typename T>
Event* make(const QJsonObject& obj)
{
    return T::fromJson(obj);
}

Event* Event::fromJson(const QJsonObject& obj)
{
    auto delegate = lookup(obj.value("type").toString(),
            "m.room.message", make<RoomMessageEvent>,
            "m.room.name", make<RoomNameEvent>,
            "m.room.aliases", make<RoomAliasesEvent>,
            "m.room.canonical_alias", make<RoomCanonicalAliasEvent>,
            "m.room.member", make<RoomMemberEvent>,
            "m.room.topic", make<RoomTopicEvent>,
            "m.typing", make<TypingEvent>,
            "m.receipt", make<ReceiptEvent>,
            /* Insert new event types BEFORE this line */
            make<UnknownEvent>
        );
    return delegate(obj);
}

bool Event::parseJson(const QJsonObject& obj)
{
    d->originalJson = QString::fromUtf8(QJsonDocument(obj).toJson());
    bool correct = (d->type != EventType::Unknown);
    if ( d->type != EventType::Unknown &&
         d->type != EventType::Typing &&
         d->type != EventType::Receipt )
    {
        if( obj.contains("event_id") )
        {
            d->id = obj.value("event_id").toString();
        } else {
            correct = false;
            qDebug() << "Event: can't find event_id";
            qDebug() << formatJson << obj;
        }
        if( obj.contains("origin_server_ts") )
        {
            d->timestamp = QDateTime::fromMSecsSinceEpoch( 
                static_cast<qint64>(obj.value("origin_server_ts").toDouble()), Qt::UTC );
        } else {
            correct = false;
            qDebug() << "Event: can't find ts";
            qDebug() << formatJson << obj;
        }
    }
    if( obj.contains("room_id") )
    {
        d->roomId = obj.value("room_id").toString();
    }
    return correct;
}

Events QMatrixClient::eventsFromJson(const QJsonArray& json)
{
    Events evs;
    evs.reserve(json.size());
    for (auto event: json)
        evs.push_back(Event::fromJson(event.toObject()));
    return evs;
}

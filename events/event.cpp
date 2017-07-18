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

#include "roommessageevent.h"
#include "roomnameevent.h"
#include "roomaliasesevent.h"
#include "roomcanonicalaliasevent.h"
#include "roommemberevent.h"
#include "roomtopicevent.h"
#include "typingevent.h"
#include "receiptevent.h"
#include "unknownevent.h"
#include "logging.h"

#include <QtCore/QJsonDocument>

using namespace QMatrixClient;

Event::Event(Type type, const QJsonObject& rep)
    : _type(type), _originalJson(rep)
{
    if (!rep.contains("content"))
    {
        qCWarning(EVENTS) << "Event without 'content' node";
        qCWarning(EVENTS) << formatJson << rep;
    }
}

QByteArray Event::originalJson() const
{
    return QJsonDocument(_originalJson).toJson();
}

QDateTime Event::toTimestamp(const QJsonValue& v)
{
    Q_ASSERT(v.isDouble() || v.isNull() || v.isUndefined());
    return QDateTime::fromMSecsSinceEpoch(
            static_cast<long long int>(v.toDouble()), Qt::UTC);
}

QStringList Event::toStringList(const QJsonValue& v)
{
    Q_ASSERT(v.isArray() || v.isNull() || v.isUndefined());

    QStringList l;
    for( const QJsonValue& e : v.toArray() )
        l.push_back(e.toString());
    return l;
}

const QJsonObject Event::contentJson() const
{
    return _originalJson["content"].toObject();
}

template <typename EventT>
EventT* make(const QJsonObject& o)
{
    return new EventT(o);
}

Event* Event::fromJson(const QJsonObject& obj)
{
    // Check more specific event types first
    if (auto e = RoomEvent::fromJson(obj))
        return e;

    return dispatch<Event*>(obj).to(obj["type"].toString(),
            "m.typing", make<TypingEvent>,
            "m.receipt", make<ReceiptEvent>,
            /* Insert new event types (except room events) BEFORE this line */
            nullptr
            );
}

RoomEvent::RoomEvent(Type type, const QJsonObject& rep)
    : Event(type, rep), _id(rep["event_id"].toString())
    , _serverTimestamp(toTimestamp(rep["origin_server_ts"]))
    , _roomId(rep["room_id"].toString())
    , _senderId(rep["sender"].toString())
    , _txnId(rep["unsigned"].toObject().value("transactionId").toString())
{
    if (_id.isEmpty())
    {
        qCWarning(EVENTS) << "Can't find event_id in a room event";
        qCWarning(EVENTS) << formatJson << rep;
    }
    if (!rep.contains("origin_server_ts"))
    {
        qCWarning(EVENTS) << "Can't find server timestamp in a room event";
        qCWarning(EVENTS) << formatJson << rep;
    }
    if (_senderId.isEmpty())
    {
        qCWarning(EVENTS) << "Can't find sender in a room event";
        qCWarning(EVENTS) << formatJson << rep;
    }
    if (!_txnId.isEmpty())
        qCDebug(EVENTS) << "Event transactionId:" << _txnId;
}

void RoomEvent::addId(const QString& id)
{
    Q_ASSERT(_id.isEmpty()); Q_ASSERT(!id.isEmpty());
    _id = id;
}

RoomEvent* RoomEvent::fromJson(const QJsonObject& obj)
{
    return dispatch<RoomEvent*>(obj).to(obj["type"].toString(),
            "m.room.message", make<RoomMessageEvent>,
            "m.room.name", make<RoomNameEvent>,
            "m.room.aliases", make<RoomAliasesEvent>,
            "m.room.canonical_alias", make<RoomCanonicalAliasEvent>,
            "m.room.member", make<RoomMemberEvent>,
            "m.room.topic", make<RoomTopicEvent>,
            /* Insert new ROOM event types BEFORE this line */
            nullptr
        );
}

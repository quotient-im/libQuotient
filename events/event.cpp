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
#include "simplestateevents.h"
#include "roommemberevent.h"
#include "roomavatarevent.h"
#include "typingevent.h"
#include "receiptevent.h"
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

QJsonObject Event::originalJsonObject() const
{
    return _originalJson;
}

const QJsonObject Event::contentJson() const
{
    return _originalJson["content"].toObject();
}

template <typename BaseEventT>
inline BaseEventT* makeIfMatches(const QJsonObject&, const QString&)
{
    return nullptr;
}

template <typename BaseEventT, typename EventT, typename... EventTs>
inline BaseEventT* makeIfMatches(const QJsonObject& o, const QString& selector)
{
    if (selector == EventT::TypeId)
        return new EventT(o);

    return makeIfMatches<BaseEventT, EventTs...>(o, selector);
}

Event* Event::fromJson(const QJsonObject& obj)
{
    // Check more specific event types first
    if (auto e = RoomEvent::fromJson(obj))
        return e;

    return makeIfMatches<Event,
        TypingEvent, ReceiptEvent>(obj, obj["type"].toString());
}

RoomEvent::RoomEvent(Type type, const QJsonObject& rep)
    : Event(type, rep), _id(rep["event_id"].toString())
    , _serverTimestamp(
          QMatrixClient::fromJson<QDateTime>(rep["origin_server_ts"]))
    , _roomId(rep["room_id"].toString())
    , _senderId(rep["sender"].toString())
    , _txnId(rep["unsigned"].toObject().value("transactionId").toString())
{
//    if (_id.isEmpty())
//    {
//        qCWarning(EVENTS) << "Can't find event_id in a room event";
//        qCWarning(EVENTS) << formatJson << rep;
//    }
//    if (!rep.contains("origin_server_ts"))
//    {
//        qCWarning(EVENTS) << "Can't find server timestamp in a room event";
//        qCWarning(EVENTS) << formatJson << rep;
//    }
//    if (_senderId.isEmpty())
//    {
//        qCWarning(EVENTS) << "Can't find sender in a room event";
//        qCWarning(EVENTS) << formatJson << rep;
//    }
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
    return makeIfMatches<RoomEvent,
        RoomMessageEvent, RoomNameEvent, RoomAliasesEvent,
        RoomCanonicalAliasEvent, RoomMemberEvent, RoomTopicEvent,
        RoomAvatarEvent, EncryptionEvent>(obj, obj["type"].toString());
}

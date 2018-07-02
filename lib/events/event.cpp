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

#include "redactionevent.h"
#include "logging.h"

#include <QtCore/QJsonDocument>

using namespace QMatrixClient;

Event::Event(Type type, const QJsonObject& json)
    : _type(type), _json(json)
{
    if (!json.contains(ContentKeyL) &&
            !json.value(UnsignedKeyL).toObject().contains(RedactedCauseKeyL))
    {
        qCWarning(EVENTS) << "Event without 'content' node";
        qCWarning(EVENTS) << formatJson << json;
    }
}

Event::Event(Type type, event_mtype_t matrixType, const QJsonObject& contentJson)
    : Event(type, basicEventJson(matrixType, contentJson))
{ }

Event::~Event() = default;

QString Event::matrixType() const
{
    return fullJson()[TypeKeyL].toString();
}

QByteArray Event::originalJson() const
{
    return QJsonDocument(_json).toJson();
}

const QJsonObject Event::contentJson() const
{
    return fullJson()[ContentKeyL].toObject();
}

const QJsonObject Event::unsignedJson() const
{
    return fullJson()[UnsignedKeyL].toObject();
}

[[gnu::unused]] static auto roomEventTypeInitialised =
        Event::factory_t::chainFactory<RoomEvent>();

RoomEvent::RoomEvent(Type type, event_mtype_t matrixType,
                     const QJsonObject& contentJson)
    : Event(type, matrixType, contentJson)
{ }

RoomEvent::RoomEvent(Type type, const QJsonObject& json)
    : Event(type, json)
{
    const auto unsignedData = json[UnsignedKeyL].toObject();
    const auto redaction = unsignedData[RedactedCauseKeyL];
    if (redaction.isObject())
    {
        _redactedBecause = makeEvent<RedactionEvent>(redaction.toObject());
        return;
    }

    _txnId = unsignedData.value("transactionId"_ls).toString();
    if (!_txnId.isEmpty())
        qCDebug(EVENTS) << "Event transactionId:" << _txnId;
}

RoomEvent::~RoomEvent() = default; // Let the smart pointer do its job

QString RoomEvent::id() const
{
    return fullJson()[EventIdKeyL].toString();
}

QDateTime RoomEvent::timestamp() const
{
    return QMatrixClient::fromJson<QDateTime>(fullJson()["origin_server_ts"_ls]);
}

QString RoomEvent::roomId() const
{
    return fullJson()["room_id"_ls].toString();
}

QString RoomEvent::senderId() const
{
    return fullJson()["sender"_ls].toString();
}

QString RoomEvent::redactionReason() const
{
    return isRedacted() ? _redactedBecause->reason() : QString{};
}

void RoomEvent::addId(const QString& newId)
{
    Q_ASSERT(id().isEmpty()); Q_ASSERT(!newId.isEmpty());
    editJson().insert(EventIdKey, newId);
}

[[gnu::unused]] static auto stateEventTypeInitialised =
        RoomEvent::factory_t::chainFactory<StateEventBase>();

bool StateEventBase::repeatsState() const
{
    const auto prevContentJson = unsignedJson().value(PrevContentKeyL);
    return fullJson().value(ContentKeyL) == prevContentJson;
}

event_type_t QMatrixClient::nextTypeId()
{
    static event_type_t _id = EventTypeTraits<void>::id;
    return ++_id;
}

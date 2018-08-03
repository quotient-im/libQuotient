/******************************************************************************
* Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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

#include "roomevent.h"

#include "redactionevent.h"
#include "converters.h"
#include "logging.h"

using namespace QMatrixClient;

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

    const auto& txnId = transactionId();
    if (!txnId.isEmpty())
        qCDebug(EVENTS) << "Event transactionId:" << txnId;
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

QString RoomEvent::transactionId() const
{
    return unsignedJson()["transaction_id"_ls].toString();
}

QString RoomEvent::stateKey() const
{
    return fullJson()["state_key"_ls].toString();
}

void RoomEvent::setTransactionId(const QString& txnId)
{
    auto unsignedData = fullJson()[UnsignedKeyL].toObject();
    unsignedData.insert(QStringLiteral("transaction_id"), txnId);
    editJson().insert(UnsignedKey, unsignedData);
    qCDebug(EVENTS) << "New event transactionId:" << txnId;
    Q_ASSERT(transactionId() == txnId);
}

void RoomEvent::addId(const QString& newId)
{
    Q_ASSERT(id().isEmpty()); Q_ASSERT(!newId.isEmpty());
    editJson().insert(EventIdKey, newId);
    qCDebug(EVENTS) << "Event txnId -> id:" << transactionId() << "->" << id();
    Q_ASSERT(id() == newId);
}

// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomevent.h"

#include "encryptedevent.h"
#include "redactionevent.h"
#include "stateevent.h"

#include "../logging_categories_p.h"

using namespace Quotient;

RoomEvent::RoomEvent(const QJsonObject& json) : Event(json)
{
    if (const auto redaction = unsignedPart<QJsonObject>(RedactedCauseKey);
        !redaction.isEmpty())
        _redactedBecause = loadEvent<RedactionEvent>(redaction);
}

RoomEvent::~RoomEvent() = default; // Let the smart pointer do its job

QString RoomEvent::displayId() const { return id().isEmpty() ? transactionId() : id(); }

QString RoomEvent::id() const { return fullJson()[EventIdKey].toString(); }

QDateTime RoomEvent::originTimestamp() const
{
    return Quotient::fromJson<QDateTime>(fullJson()["origin_server_ts"_L1]);
}

QString RoomEvent::roomId() const
{
    return fullJson()[RoomIdKey].toString();
}

QString RoomEvent::senderId() const
{
    return fullJson()[SenderKey].toString();
}

QString RoomEvent::redactionReason() const
{
    return isRedacted() ? _redactedBecause->reason() : QString {};
}

QString RoomEvent::transactionId() const
{
    return unsignedPart<QString>("transaction_id"_L1);
}

bool RoomEvent::isStateEvent() const { return is<StateEvent>(); }

QString RoomEvent::stateKey() const
{
    return fullJson()[StateKeyKey].toString();
}

void RoomEvent::setRoomId(const QString& roomId)
{
    editJson().insert(RoomIdKey, roomId);
}

void RoomEvent::setSender(const QString& senderId)
{
    editJson().insert(SenderKey, senderId);
}

void RoomEvent::setTransactionId(const QString& txnId)
{
    auto unsignedData = fullJson()[UnsignedKey].toObject();
    unsignedData.insert("transaction_id"_L1, txnId);
    editJson().insert(UnsignedKey, unsignedData);
    Q_ASSERT(transactionId() == txnId);
}

void RoomEvent::addId(const QString& newId)
{
    Q_ASSERT(id().isEmpty());
    Q_ASSERT(!newId.isEmpty());
    editJson().insert(EventIdKey, newId);
    qCDebug(EVENTS) << "Event txnId -> id:" << transactionId() << "->" << id();
    Q_ASSERT(id() == newId);
}

void RoomEvent::dumpTo(QDebug dbg) const
{
    Event::dumpTo(dbg);
    dbg << " (made at " << originTimestamp().toString(Qt::ISODate) << ')';
}

void RoomEvent::setOriginalEvent(event_ptr_tt<EncryptedEvent>&& originalEvent)
{
    _originalEvent = std::move(originalEvent);
}

const QJsonObject RoomEvent::encryptedJson() const
{
    if(!_originalEvent) {
        return {};
    }
    return _originalEvent->fullJson();
}

namespace {
bool containsEventType(const auto& haystack, const auto& needle)
{
    return std::ranges::any_of(haystack, [needle](const AbstractEventMetaType* candidate) {
        return candidate->matrixId == needle || containsEventType(candidate->derivedTypes(), needle);
    });
}
}

bool Quotient::isStateEvent(const QString& eventTypeId)
{
    return containsEventType(StateEvent::BaseMetaType.derivedTypes(), eventTypeId);
}

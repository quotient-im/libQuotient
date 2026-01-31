// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomevent.h"

#include "encryptedevent.h"
#include "redactionevent.h"
#include "stateevent.h"
#include "../logging_categories_p.h"
#include "../ranges_extras.h"

using namespace Quotient;

RoomEvent::RoomEvent(const QJsonObject &json) : Event(json), _id(fullJson()[EventIdKey].toString())
{
    if (const auto redaction = unsignedPart<QJsonObject>(RedactedCauseKey);
        !redaction.isEmpty())
        _redactedBecause = loadEvent<RedactionEvent>(redaction);
}

RoomEvent::~RoomEvent() = default; // Let the smart pointer do its job

QString RoomEvent::displayId() const { return id().isEmpty() ? transactionId() : id(); }

QString RoomEvent::id() const { return _id; }

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
    _id = newId;
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
bool containsEventType(std::span<const AbstractEventMetaType *const> haystack, const QString &needle)
{
    return std::ranges::any_of(haystack, [needle](const AbstractEventMetaType *candidate) {
        return rangeContains(candidate->matrixIds, needle)
               || containsEventType(candidate->derivedTypes(), needle);
    });
}
}

bool Quotient::isStateEvent(const QString& eventTypeId)
{
    return containsEventType(StateEvent::BaseMetaType.derivedTypes(), eventTypeId);
}

bool RoomEvent::isReply(bool includeFallbacks) const
{
    const auto relation = relatesTo();
    return relation
           && (relation->type == EventRelation::ReplyType
               || (relation->type == EventRelation::ThreadType
                   && (relation->isFallingBack == false || includeFallbacks)));
}

QString RoomEvent::replyEventId(bool includeFallbacks) const
{
    if (const auto relation = relatesTo()) {
        if (relation->type == EventRelation::ReplyType) {
            return relation->eventId;
        } else if (relation->type == EventRelation::ThreadType
                   && (relation->isFallingBack == false || includeFallbacks)) {
            return relation->inThreadReplyEventId;
        }
    }
    return {};
}

std::optional<EventRelation> RoomEvent::relatesTo() const
{
    return contentPart<std::optional<EventRelation>>(RelatesToKey);
}

void RoomEvent::setRelation(const EventRelation &er)
{
    replaceSubvalue(editJson(), ContentKey, RelatesToKey, toJson(er));
}

void RoomEvent::clearRelation()
{
    editSubobject(editJson(), ContentKey, [](QJsonObject& content) {
        content.remove(RelatesToKey);
    });
}

QJsonObject RoomEvent::relationsToThis() const
{
    return unsignedPart<QJsonObject>(RelationsKey);
}

bool RoomEvent::hasRelationship(EventRelation::typeid_t relationTypeId) const
{
    return relationsToThis().contains(relationTypeId);
}

QString RoomEvent::replacedEvent() const
{
    if (is<StateEvent>())
        return {}; // State events can't be replaced

    const auto er = relatesTo();
    return er && er->type == EventRelation::ReplacementType && contentJson().contains(NewContentKey)
               ? er->eventId
               : QString();
}

bool RoomEvent::isReplaced() const
{
    return hasRelationship(EventRelation::ReplacementType);
}

QString RoomEvent::replacedBy() const
{
    return relationsToThis().value(EventRelation::ReplacementType)[EventIdKey].toString();
}

bool RoomEvent::isThreaded() const
{
    const auto relation = relatesTo();
    return (relation && relation->type == EventRelation::ThreadType)
           || hasRelationship(EventRelation::ThreadType);
}

QString RoomEvent::threadRootEventId() const
{
    if (const auto relation = relatesTo(); relation && relation->type == EventRelation::ThreadType) {
        return relation->eventId;
    }
    if (hasRelationship(EventRelation::ThreadType)) {
        return id();
    }
    return {};
}

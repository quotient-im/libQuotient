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
#include "accountdataevents.h"
#include "directchatevent.h"
#include "redactionevent.h"
#include "logging.h"

#include <QtCore/QJsonDocument>

using namespace QMatrixClient;

Event::Event(Type type, const QJsonObject& rep)
    : _type(type), _originalJson(rep)
{
    if (!rep.contains("content") &&
            !rep.value("unsigned").toObject().contains("redacted_because"))
    {
        qCWarning(EVENTS) << "Event without 'content' node";
        qCWarning(EVENTS) << formatJson << rep;
    }
}

Event::~Event() = default;

QString Event::jsonType() const
{
    return originalJsonObject().value("type").toString();
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
inline event_ptr_tt<BaseEventT> makeIfMatches(const QJsonObject&, const QString&)
{
    return nullptr;
}

template <typename BaseEventT, typename EventT, typename... EventTs>
inline event_ptr_tt<BaseEventT> makeIfMatches(const QJsonObject& o,
                                              const QString& selector)
{
    if (selector == EventT::typeId())
        return _impl::create<EventT>(o);

    return makeIfMatches<BaseEventT, EventTs...>(o, selector);
}

template <>
EventPtr _impl::doMakeEvent<Event>(const QJsonObject& obj)
{
    // Check more specific event types first
    if (auto e = doMakeEvent<RoomEvent>(obj))
        return e;

    return makeIfMatches<Event,
        TypingEvent, ReceiptEvent, TagEvent, ReadMarkerEvent, DirectChatEvent>(
                    obj, obj["type"].toString());
}

RoomEvent::RoomEvent(Event::Type type) : Event(type) { }

RoomEvent::RoomEvent(Type type, const QJsonObject& rep)
    : Event(type, rep)
    , _id(rep["event_id"].toString())
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
    auto unsignedData = rep["unsigned"].toObject();
    auto redaction = unsignedData.value("redacted_because");
    if (redaction.isObject())
    {
        _redactedBecause = _impl::create<RedactionEvent>(redaction.toObject());
        return;
    }

    _txnId = unsignedData.value("transactionId").toString();
    if (!_txnId.isEmpty())
        qCDebug(EVENTS) << "Event transactionId:" << _txnId;
}

RoomEvent::~RoomEvent() = default; // Let the smart pointer do its job

QDateTime RoomEvent::timestamp() const
{
    return QMatrixClient::fromJson<QDateTime>(
                originalJsonObject().value("origin_server_ts"));
}

QString RoomEvent::roomId() const
{
    return originalJsonObject().value("room_id").toString();
}

QString RoomEvent::senderId() const
{
    return originalJsonObject().value("sender").toString();
}

QString RoomEvent::redactionReason() const
{
    return isRedacted() ? _redactedBecause->reason() : QString{};
}

void RoomEvent::addId(const QString& id)
{
    Q_ASSERT(_id.isEmpty()); Q_ASSERT(!id.isEmpty());
    _id = id;
}

template <>
RoomEventPtr _impl::doMakeEvent(const QJsonObject& obj)
{
    // Check more specific event types first
    if (auto e = doMakeEvent<StateEventBase>(obj))
        return e;

    return makeIfMatches<RoomEvent,
        RoomMessageEvent, RedactionEvent>(obj, obj["type"].toString());
}

bool StateEventBase::repeatsState() const
{
    auto contentJson = originalJsonObject().value("content");
    auto prevContentJson = originalJsonObject().value("unsigned")
                                .toObject().value("prev_content");
    return contentJson == prevContentJson;
}

template<>
StateEventPtr _impl::doMakeEvent<StateEventBase>(const QJsonObject& obj)
{
    return makeIfMatches<StateEventBase,
        RoomNameEvent, RoomAliasesEvent,
        RoomCanonicalAliasEvent, RoomMemberEvent, RoomTopicEvent,
        RoomAvatarEvent, EncryptionEvent>(obj, obj["type"].toString());

}

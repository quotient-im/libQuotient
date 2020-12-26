/******************************************************************************
 * SPDX-FileCopyrightText: 2019 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "roomevent.h"

namespace Quotient {

struct EventRelation {
    using reltypeid_t = const char*;
    static constexpr reltypeid_t Reply() { return "m.in_reply_to"; }
    static constexpr reltypeid_t Annotation() { return "m.annotation"; }
    static constexpr reltypeid_t Replacement() { return "m.replace"; }

    QString type;
    QString eventId;
    QString key = {}; // Only used for m.annotation for now

    static EventRelation replyTo(QString eventId)
    {
        return { Reply(), std::move(eventId) };
    }
    static EventRelation annotate(QString eventId, QString key)
    {
        return { Annotation(), std::move(eventId), std::move(key) };
    }
    static EventRelation replace(QString eventId)
    {
        return { Replacement(), std::move(eventId) };
    }
};
template <>
struct JsonObjectConverter<EventRelation> {
    static void dumpTo(QJsonObject& jo, const EventRelation& pod);
    static void fillFrom(const QJsonObject& jo, EventRelation& pod);
};

class ReactionEvent : public RoomEvent {
public:
    DEFINE_EVENT_TYPEID("m.reaction", ReactionEvent)

    explicit ReactionEvent(const EventRelation& value)
        : RoomEvent(typeId(), matrixTypeId(),
                    { { QStringLiteral("m.relates_to"), toJson(value) } })
    {}
    explicit ReactionEvent(const QJsonObject& obj) : RoomEvent(typeId(), obj) {}
    EventRelation relation() const
    {
        return content<EventRelation>(QStringLiteral("m.relates_to"));
    }
};
REGISTER_EVENT_TYPE(ReactionEvent)

} // namespace Quotient

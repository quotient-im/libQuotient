/******************************************************************************
 * Copyright (C) 2019 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include "roomevent.h"

namespace QMatrixClient {

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

private:
    EventRelation _relation;
};
REGISTER_EVENT_TYPE(ReactionEvent)

} // namespace QMatrixClient

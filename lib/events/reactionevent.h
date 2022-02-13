// SPDX-FileCopyrightText: 2019 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"
#include "eventrelation.h"

namespace Quotient {

class QUOTIENT_API ReactionEvent : public RoomEvent {
public:
    DEFINE_EVENT_TYPEID("m.reaction", ReactionEvent)

    explicit ReactionEvent(const EventRelation& value)
        : RoomEvent(typeId(), matrixTypeId(),
                    { { QStringLiteral("m.relates_to"), toJson(value) } })
    {}
    explicit ReactionEvent(const QJsonObject& obj) : RoomEvent(typeId(), obj) {}
    EventRelation relation() const
    {
        return contentPart<EventRelation>(RelatesToKey);
    }
};
REGISTER_EVENT_TYPE(ReactionEvent)

} // namespace Quotient

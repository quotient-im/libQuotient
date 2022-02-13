// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"
#include "quotient_common.h"

namespace Quotient {
class QUOTIENT_API RoomCreateEvent : public StateEventBase {
public:
    DEFINE_EVENT_TYPEID("m.room.create", RoomCreateEvent)

    explicit RoomCreateEvent(const QJsonObject& obj)
        : StateEventBase(typeId(), obj)
    {}

    struct Predecessor {
        QString roomId;
        QString eventId;
    };

    bool isFederated() const;
    QString version() const;
    Predecessor predecessor() const;
    bool isUpgrade() const;
    RoomType roomType() const;
};
REGISTER_EVENT_TYPE(RoomCreateEvent)
} // namespace Quotient

// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"

namespace Quotient {
class RoomTombstoneEvent : public StateEventBase {
public:
    DEFINE_EVENT_TYPEID("m.room.tombstone", RoomTombstoneEvent)

    explicit RoomTombstoneEvent() : StateEventBase(typeId(), matrixTypeId()) {}
    explicit RoomTombstoneEvent(const QJsonObject& obj)
        : StateEventBase(typeId(), obj)
    {}

    QString serverMessage() const;
    QString successorRoomId() const;
};
REGISTER_EVENT_TYPE(RoomTombstoneEvent)
} // namespace Quotient

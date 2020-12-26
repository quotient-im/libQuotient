/******************************************************************************
 * SPDX-FileCopyrightText: 2019 QMatrixClient project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "stateevent.h"

namespace Quotient {
class RoomCreateEvent : public StateEventBase {
public:
    DEFINE_EVENT_TYPEID("m.room.create", RoomCreateEvent)

    explicit RoomCreateEvent() : StateEventBase(typeId(), matrixTypeId()) {}
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
};
REGISTER_EVENT_TYPE(RoomCreateEvent)
} // namespace Quotient

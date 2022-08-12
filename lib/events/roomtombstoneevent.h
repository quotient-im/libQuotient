// SPDX-FileCopyrightText: 2019 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"

namespace Quotient {
class QUOTIENT_API RoomTombstoneEvent : public StateEventBase {
public:
    QUO_EVENT(RoomTombstoneEvent, "m.room.tombstone")

    using StateEventBase::StateEventBase;

    QString serverMessage() const;
    QString successorRoomId() const;
};
} // namespace Quotient

// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"

namespace Quotient {
class QUOTIENT_API ServerAclEvent : public StateEvent {
public:
    QUO_EVENT(ServerAclEvent, "m.room.server_acl")

    using StateEvent::StateEvent;

    QStringList allow() const;
    bool allowIpLiterals() const;
    QStringList deny() const;
};
} // namespace Quotient

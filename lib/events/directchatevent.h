// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

namespace Quotient {
class QUOTIENT_API DirectChatEvent : public Event {
public:
    DEFINE_EVENT_TYPEID("m.direct", DirectChatEvent)

    explicit DirectChatEvent(const QJsonObject& obj) : Event(typeId(), obj) {}

    QMultiHash<QString, QString> usersToDirectChats() const;
};
REGISTER_EVENT_TYPE(DirectChatEvent)
} // namespace Quotient

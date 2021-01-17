// SPDX-FileCopyrightText: 2017 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

namespace Quotient {
class TypingEvent : public Event {
public:
    DEFINE_EVENT_TYPEID("m.typing", TypingEvent)

    explicit TypingEvent(const QJsonObject& obj) : Event(typeId(), obj) {}

    QStringList users() const;
};
REGISTER_EVENT_TYPE(TypingEvent)
} // namespace Quotient

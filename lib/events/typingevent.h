/******************************************************************************
 * SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

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

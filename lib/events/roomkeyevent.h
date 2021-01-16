// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

namespace Quotient {
class RoomKeyEvent : public Event
{
public:
    DEFINE_EVENT_TYPEID("m.room_key", RoomKeyEvent)

    explicit RoomKeyEvent(const QJsonObject& obj);

    QString algorithm() const { return content<QString>("algorithm"_ls); }
    QString roomId() const { return content<QString>(RoomIdKeyL); }
    QString sessionId() const { return content<QString>("session_id"_ls); }
    QString sessionKey() const { return content<QString>("session_key"_ls); }
};
REGISTER_EVENT_TYPE(RoomKeyEvent)
} // namespace Quotient

// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

namespace Quotient {
class QUOTIENT_API RoomKeyEvent : public Event
{
public:
    DEFINE_EVENT_TYPEID("m.room_key", RoomKeyEvent)

    explicit RoomKeyEvent(const QJsonObject& obj);
    explicit RoomKeyEvent(const QString& algorithm, const QString& roomId, const QString &sessionId, const QString& sessionKey, const QString& senderId);

    QString algorithm() const { return contentPart<QString>("algorithm"_ls); }
    QString roomId() const { return contentPart<QString>(RoomIdKeyL); }
    QString sessionId() const { return contentPart<QString>("session_id"_ls); }
    QByteArray sessionKey() const
    {
        return contentPart<QString>("session_key"_ls).toLatin1();
    }
};
REGISTER_EVENT_TYPE(RoomKeyEvent)
} // namespace Quotient

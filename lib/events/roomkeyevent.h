// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

namespace Quotient {
class QUOTIENT_API RoomKeyEvent : public Event
{
public:
    QUO_EVENT(RoomKeyEvent, "m.room_key")

    explicit RoomKeyEvent(const QJsonObject& obj);
    explicit RoomKeyEvent(const QString& algorithm, const QString& roomId,
                          const QString& sessionId, const QString& sessionKey);

    QUO_CONTENT_GETTER(QString, algorithm)
    QUO_CONTENT_GETTER(QString, roomId)
    QUO_CONTENT_GETTER(QString, sessionId)
    QByteArray sessionKey() const
    {
        return contentPart<QString>("session_key"_ls).toLatin1();
    }
};
} // namespace Quotient

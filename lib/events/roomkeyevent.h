// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

namespace Quotient {
class QUOTIENT_API RoomKeyEvent : public Event
{
public:
    QUO_EVENT(RoomKeyEvent, "m.room_key")

    using Event::Event;
    explicit RoomKeyEvent(const QString& algorithm, const QString& roomId,
                          const QString& sessionId, const QString& sessionKey)
        : Event(basicJson(TypeId, {
                                      { "algorithm", algorithm },
                                      { "room_id", roomId },
                                      { "session_id", sessionId },
                                      { "session_key", sessionKey },
                                  }))
    {}

    QUO_CONTENT_GETTER(QString, algorithm)
    QUO_CONTENT_GETTER(QString, roomId)
    QUO_CONTENT_GETTER(QString, sessionId)
    QUO_LATIN1_CONTENT_GETTER_X(sessionKey, "session_key"_ls)
};
} // namespace Quotient

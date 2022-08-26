// SPDX-FileCopyrightText: 2019 Alexey Andreyev <aa13q@ya.ru>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roomkeyevent.h"

using namespace Quotient;

RoomKeyEvent::RoomKeyEvent(const QJsonObject &obj) : Event(TypeId, obj)
{
    if (roomId().isEmpty())
        qCWarning(E2EE) << "Room key event has empty room id";
}

RoomKeyEvent::RoomKeyEvent(const QString& algorithm, const QString& roomId,
                           const QString& sessionId, const QString& sessionKey)
    : Event(TypeId, basicJson(TypeId, {
                                          { "algorithm", algorithm },
                                          { "room_id", roomId },
                                          { "session_id", sessionId },
                                          { "session_key", sessionKey },
                                      }))
{}

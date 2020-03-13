#pragma once

#include "event.h"

namespace Quotient {
class RoomKeyEvent : public Event
{
public:
    DEFINE_EVENT_TYPEID("m.room_key", RoomKeyEvent)

    RoomKeyEvent(const QJsonObject& obj);

    const QString algorithm() const { return _algorithm; }
    const QString roomId() const { return _roomId; }
    const QString sessionId() const { return _sessionId; }
    const QString sessionKey() const { return _sessionKey; }

private:
    QString _algorithm;
    QString _roomId;
    QString _sessionId;
    QString _sessionKey;
};
REGISTER_EVENT_TYPE(RoomKeyEvent)
} // namespace Quotient

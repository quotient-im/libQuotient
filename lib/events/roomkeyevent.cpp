#include "roomkeyevent.h"

using namespace Quotient;

RoomKeyEvent::RoomKeyEvent(const QJsonObject &obj) : Event(typeId(), obj)
{
    _algorithm = contentJson()["algorithm"_ls].toString();
    _roomId = contentJson()["room_id"_ls].toString();
    _sessionId = contentJson()["session_id"_ls].toString();
    _sessionKey = contentJson()["session_key"_ls].toString();
}

#include "roomkeyevent.h"

using namespace Quotient;

RoomKeyEvent::RoomKeyEvent(const QJsonObject &obj) : Event(typeId(), obj)
{
    if (roomId().isEmpty())
        qCWarning(E2EE) << "Room key event has empty room id";
}

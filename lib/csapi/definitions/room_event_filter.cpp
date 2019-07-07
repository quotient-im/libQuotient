/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_event_filter.h"

using namespace Quotient;

void JsonObjectConverter<RoomEventFilter>::dumpTo(QJsonObject& jo,
                                                  const RoomEventFilter& pod)
{
    fillJson<EventFilter>(jo, pod);
    addParam<IfNotEmpty>(jo, QStringLiteral("not_rooms"), pod.notRooms);
    addParam<IfNotEmpty>(jo, QStringLiteral("rooms"), pod.rooms);
    addParam<IfNotEmpty>(jo, QStringLiteral("contains_url"), pod.containsUrl);
}

void JsonObjectConverter<RoomEventFilter>::fillFrom(const QJsonObject& jo,
                                                    RoomEventFilter& result)
{
    fillFromJson<EventFilter>(jo, result);
    fromJson(jo.value("not_rooms"_ls), result.notRooms);
    fromJson(jo.value("rooms"_ls), result.rooms);
    fromJson(jo.value("contains_url"_ls), result.containsUrl);
}

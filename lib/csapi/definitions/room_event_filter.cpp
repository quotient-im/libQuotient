/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_event_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const RoomEventFilter& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("not_rooms"), pod.notRooms);
    addParam<IfNotEmpty>(jo, QStringLiteral("rooms"), pod.rooms);
    addParam<IfNotEmpty>(jo, QStringLiteral("contains_url"), pod.containsUrl);
    return jo;
}

RoomEventFilter FromJsonObject<RoomEventFilter>::operator()(const QJsonObject& jo) const
{
    RoomEventFilter result;
    result.notRooms =
        fromJson<QStringList>(jo.value("not_rooms"_ls));
    result.rooms =
        fromJson<QStringList>(jo.value("rooms"_ls));
    result.containsUrl =
        fromJson<bool>(jo.value("contains_url"_ls));

    return result;
}


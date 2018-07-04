/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_event_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const RoomEventFilter& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("not_rooms"), pod.notRooms);
    addParam<IfNotEmpty>(_json, QStringLiteral("rooms"), pod.rooms);
    addParam<IfNotEmpty>(_json, QStringLiteral("contains_url"), pod.containsUrl);
    return _json;
}

RoomEventFilter FromJson<RoomEventFilter>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    RoomEventFilter result;
    result.notRooms =
        fromJson<QStringList>(_json.value("not_rooms"_ls));
    result.rooms =
        fromJson<QStringList>(_json.value("rooms"_ls));
    result.containsUrl =
        fromJson<bool>(_json.value("contains_url"_ls));
    
    return result;
}


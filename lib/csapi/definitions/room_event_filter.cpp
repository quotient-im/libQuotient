/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_event_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const RoomEventFilter& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, "not_rooms", pod.notRooms);
    addParam<IfNotEmpty>(_json, "rooms", pod.rooms);
    addParam<IfNotEmpty>(_json, "contains_url", pod.containsUrl);
    return _json;
}

RoomEventFilter FromJson<RoomEventFilter>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    RoomEventFilter result;
    result.notRooms =
        fromJson<QStringList>(_json.value("not_rooms"));
    result.rooms =
        fromJson<QStringList>(_json.value("rooms"));
    result.containsUrl =
        fromJson<bool>(_json.value("contains_url"));
    
    return result;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sync_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const RoomFilter& pod)
{
    QJsonObject _json;
    addToJson<IfNotEmpty>(_json, "not_rooms", pod.notRooms);
    addToJson<IfNotEmpty>(_json, "rooms", pod.rooms);
    addToJson<IfNotEmpty>(_json, "ephemeral", pod.ephemeral);
    addToJson<IfNotEmpty>(_json, "include_leave", pod.includeLeave);
    addToJson<IfNotEmpty>(_json, "state", pod.state);
    addToJson<IfNotEmpty>(_json, "timeline", pod.timeline);
    addToJson<IfNotEmpty>(_json, "account_data", pod.accountData);
    return _json;
}

RoomFilter FromJson<RoomFilter>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    RoomFilter result;
    result.notRooms =
        fromJson<QStringList>(_json.value("not_rooms"));
    result.rooms =
        fromJson<QStringList>(_json.value("rooms"));
    result.ephemeral =
        fromJson<RoomEventFilter>(_json.value("ephemeral"));
    result.includeLeave =
        fromJson<bool>(_json.value("include_leave"));
    result.state =
        fromJson<RoomEventFilter>(_json.value("state"));
    result.timeline =
        fromJson<RoomEventFilter>(_json.value("timeline"));
    result.accountData =
        fromJson<RoomEventFilter>(_json.value("account_data"));
    
    return result;
}

QJsonObject QMatrixClient::toJson(const SyncFilter& pod)
{
    QJsonObject _json;
    addToJson<IfNotEmpty>(_json, "event_fields", pod.eventFields);
    addToJson<IfNotEmpty>(_json, "event_format", pod.eventFormat);
    addToJson<IfNotEmpty>(_json, "presence", pod.presence);
    addToJson<IfNotEmpty>(_json, "account_data", pod.accountData);
    addToJson<IfNotEmpty>(_json, "room", pod.room);
    return _json;
}

SyncFilter FromJson<SyncFilter>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    SyncFilter result;
    result.eventFields =
        fromJson<QStringList>(_json.value("event_fields"));
    result.eventFormat =
        fromJson<QString>(_json.value("event_format"));
    result.presence =
        fromJson<Filter>(_json.value("presence"));
    result.accountData =
        fromJson<Filter>(_json.value("account_data"));
    result.room =
        fromJson<RoomFilter>(_json.value("room"));
    
    return result;
}


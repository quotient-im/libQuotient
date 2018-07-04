/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sync_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const RoomFilter& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("not_rooms"), pod.notRooms);
    addParam<IfNotEmpty>(_json, QStringLiteral("rooms"), pod.rooms);
    addParam<IfNotEmpty>(_json, QStringLiteral("ephemeral"), pod.ephemeral);
    addParam<IfNotEmpty>(_json, QStringLiteral("include_leave"), pod.includeLeave);
    addParam<IfNotEmpty>(_json, QStringLiteral("state"), pod.state);
    addParam<IfNotEmpty>(_json, QStringLiteral("timeline"), pod.timeline);
    addParam<IfNotEmpty>(_json, QStringLiteral("account_data"), pod.accountData);
    return _json;
}

RoomFilter FromJson<RoomFilter>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    RoomFilter result;
    result.notRooms =
        fromJson<QStringList>(_json.value("not_rooms"_ls));
    result.rooms =
        fromJson<QStringList>(_json.value("rooms"_ls));
    result.ephemeral =
        fromJson<RoomEventFilter>(_json.value("ephemeral"_ls));
    result.includeLeave =
        fromJson<bool>(_json.value("include_leave"_ls));
    result.state =
        fromJson<RoomEventFilter>(_json.value("state"_ls));
    result.timeline =
        fromJson<RoomEventFilter>(_json.value("timeline"_ls));
    result.accountData =
        fromJson<RoomEventFilter>(_json.value("account_data"_ls));
    
    return result;
}

QJsonObject QMatrixClient::toJson(const SyncFilter& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("event_fields"), pod.eventFields);
    addParam<IfNotEmpty>(_json, QStringLiteral("event_format"), pod.eventFormat);
    addParam<IfNotEmpty>(_json, QStringLiteral("presence"), pod.presence);
    addParam<IfNotEmpty>(_json, QStringLiteral("account_data"), pod.accountData);
    addParam<IfNotEmpty>(_json, QStringLiteral("room"), pod.room);
    return _json;
}

SyncFilter FromJson<SyncFilter>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    SyncFilter result;
    result.eventFields =
        fromJson<QStringList>(_json.value("event_fields"_ls));
    result.eventFormat =
        fromJson<QString>(_json.value("event_format"_ls));
    result.presence =
        fromJson<Filter>(_json.value("presence"_ls));
    result.accountData =
        fromJson<Filter>(_json.value("account_data"_ls));
    result.room =
        fromJson<RoomFilter>(_json.value("room"_ls));
    
    return result;
}


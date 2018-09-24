/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sync_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const RoomFilter& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("not_rooms"), pod.notRooms);
    addParam<IfNotEmpty>(jo, QStringLiteral("rooms"), pod.rooms);
    addParam<IfNotEmpty>(jo, QStringLiteral("ephemeral"), pod.ephemeral);
    addParam<IfNotEmpty>(jo, QStringLiteral("include_leave"), pod.includeLeave);
    addParam<IfNotEmpty>(jo, QStringLiteral("state"), pod.state);
    addParam<IfNotEmpty>(jo, QStringLiteral("timeline"), pod.timeline);
    addParam<IfNotEmpty>(jo, QStringLiteral("account_data"), pod.accountData);
    return jo;
}

RoomFilter FromJsonObject<RoomFilter>::operator()(const QJsonObject& jo) const
{
    RoomFilter result;
    result.notRooms =
        fromJson<QStringList>(jo.value("not_rooms"_ls));
    result.rooms =
        fromJson<QStringList>(jo.value("rooms"_ls));
    result.ephemeral =
        fromJson<RoomEventFilter>(jo.value("ephemeral"_ls));
    result.includeLeave =
        fromJson<bool>(jo.value("include_leave"_ls));
    result.state =
        fromJson<RoomEventFilter>(jo.value("state"_ls));
    result.timeline =
        fromJson<RoomEventFilter>(jo.value("timeline"_ls));
    result.accountData =
        fromJson<RoomEventFilter>(jo.value("account_data"_ls));

    return result;
}

QJsonObject QMatrixClient::toJson(const SyncFilter& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("event_fields"), pod.eventFields);
    addParam<IfNotEmpty>(jo, QStringLiteral("event_format"), pod.eventFormat);
    addParam<IfNotEmpty>(jo, QStringLiteral("presence"), pod.presence);
    addParam<IfNotEmpty>(jo, QStringLiteral("account_data"), pod.accountData);
    addParam<IfNotEmpty>(jo, QStringLiteral("room"), pod.room);
    return jo;
}

SyncFilter FromJsonObject<SyncFilter>::operator()(const QJsonObject& jo) const
{
    SyncFilter result;
    result.eventFields =
        fromJson<QStringList>(jo.value("event_fields"_ls));
    result.eventFormat =
        fromJson<QString>(jo.value("event_format"_ls));
    result.presence =
        fromJson<Filter>(jo.value("presence"_ls));
    result.accountData =
        fromJson<Filter>(jo.value("account_data"_ls));
    result.room =
        fromJson<RoomFilter>(jo.value("room"_ls));

    return result;
}


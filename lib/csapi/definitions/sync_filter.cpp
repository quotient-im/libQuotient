/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sync_filter.h"

using namespace Quotient;

void JsonObjectConverter<StateFilter>::dumpTo(QJsonObject& jo,
                                              const StateFilter& pod)
{
    fillJson<RoomEventFilter>(jo, pod);
    addParam<IfNotEmpty>(jo, QStringLiteral("lazy_load_members"),
                         pod.lazyLoadMembers);
    addParam<IfNotEmpty>(jo, QStringLiteral("include_redundant_members"),
                         pod.includeRedundantMembers);
}

void JsonObjectConverter<StateFilter>::fillFrom(const QJsonObject& jo,
                                                StateFilter& result)
{
    fillFromJson<RoomEventFilter>(jo, result);
    fromJson(jo.value("lazy_load_members"_ls), result.lazyLoadMembers);
    fromJson(jo.value("include_redundant_members"_ls),
             result.includeRedundantMembers);
}

void JsonObjectConverter<RoomFilter>::dumpTo(QJsonObject& jo,
                                             const RoomFilter& pod)
{
    addParam<IfNotEmpty>(jo, QStringLiteral("not_rooms"), pod.notRooms);
    addParam<IfNotEmpty>(jo, QStringLiteral("rooms"), pod.rooms);
    addParam<IfNotEmpty>(jo, QStringLiteral("ephemeral"), pod.ephemeral);
    addParam<IfNotEmpty>(jo, QStringLiteral("include_leave"), pod.includeLeave);
    addParam<IfNotEmpty>(jo, QStringLiteral("state"), pod.state);
    addParam<IfNotEmpty>(jo, QStringLiteral("timeline"), pod.timeline);
    addParam<IfNotEmpty>(jo, QStringLiteral("account_data"), pod.accountData);
}

void JsonObjectConverter<RoomFilter>::fillFrom(const QJsonObject& jo,
                                               RoomFilter& result)
{
    fromJson(jo.value("not_rooms"_ls), result.notRooms);
    fromJson(jo.value("rooms"_ls), result.rooms);
    fromJson(jo.value("ephemeral"_ls), result.ephemeral);
    fromJson(jo.value("include_leave"_ls), result.includeLeave);
    fromJson(jo.value("state"_ls), result.state);
    fromJson(jo.value("timeline"_ls), result.timeline);
    fromJson(jo.value("account_data"_ls), result.accountData);
}

void JsonObjectConverter<Filter>::dumpTo(QJsonObject& jo, const Filter& pod)
{
    addParam<IfNotEmpty>(jo, QStringLiteral("event_fields"), pod.eventFields);
    addParam<IfNotEmpty>(jo, QStringLiteral("event_format"), pod.eventFormat);
    addParam<IfNotEmpty>(jo, QStringLiteral("presence"), pod.presence);
    addParam<IfNotEmpty>(jo, QStringLiteral("account_data"), pod.accountData);
    addParam<IfNotEmpty>(jo, QStringLiteral("room"), pod.room);
}

void JsonObjectConverter<Filter>::fillFrom(const QJsonObject& jo, Filter& result)
{
    fromJson(jo.value("event_fields"_ls), result.eventFields);
    fromJson(jo.value("event_format"_ls), result.eventFormat);
    fromJson(jo.value("presence"_ls), result.presence);
    fromJson(jo.value("account_data"_ls), result.accountData);
    fromJson(jo.value("room"_ls), result.room);
}

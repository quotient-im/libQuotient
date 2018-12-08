/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "public_rooms_response.h"

using namespace QMatrixClient;

void JsonObjectConverter<PublicRoomsChunk>::dumpTo(
        QJsonObject& jo, const PublicRoomsChunk& pod)
{
    addParam<IfNotEmpty>(jo, QStringLiteral("aliases"), pod.aliases);
    addParam<IfNotEmpty>(jo, QStringLiteral("canonical_alias"), pod.canonicalAlias);
    addParam<IfNotEmpty>(jo, QStringLiteral("name"), pod.name);
    addParam<>(jo, QStringLiteral("num_joined_members"), pod.numJoinedMembers);
    addParam<>(jo, QStringLiteral("room_id"), pod.roomId);
    addParam<IfNotEmpty>(jo, QStringLiteral("topic"), pod.topic);
    addParam<>(jo, QStringLiteral("world_readable"), pod.worldReadable);
    addParam<>(jo, QStringLiteral("guest_can_join"), pod.guestCanJoin);
    addParam<IfNotEmpty>(jo, QStringLiteral("avatar_url"), pod.avatarUrl);
}

void JsonObjectConverter<PublicRoomsChunk>::fillFrom(
    const QJsonObject& jo, PublicRoomsChunk& result)
{
    fromJson(jo.value("aliases"_ls), result.aliases);
    fromJson(jo.value("canonical_alias"_ls), result.canonicalAlias);
    fromJson(jo.value("name"_ls), result.name);
    fromJson(jo.value("num_joined_members"_ls), result.numJoinedMembers);
    fromJson(jo.value("room_id"_ls), result.roomId);
    fromJson(jo.value("topic"_ls), result.topic);
    fromJson(jo.value("world_readable"_ls), result.worldReadable);
    fromJson(jo.value("guest_can_join"_ls), result.guestCanJoin);
    fromJson(jo.value("avatar_url"_ls), result.avatarUrl);
}

void JsonObjectConverter<PublicRoomsResponse>::dumpTo(
        QJsonObject& jo, const PublicRoomsResponse& pod)
{
    addParam<>(jo, QStringLiteral("chunk"), pod.chunk);
    addParam<IfNotEmpty>(jo, QStringLiteral("next_batch"), pod.nextBatch);
    addParam<IfNotEmpty>(jo, QStringLiteral("prev_batch"), pod.prevBatch);
    addParam<IfNotEmpty>(jo, QStringLiteral("total_room_count_estimate"), pod.totalRoomCountEstimate);
}

void JsonObjectConverter<PublicRoomsResponse>::fillFrom(
    const QJsonObject& jo, PublicRoomsResponse& result)
{
    fromJson(jo.value("chunk"_ls), result.chunk);
    fromJson(jo.value("next_batch"_ls), result.nextBatch);
    fromJson(jo.value("prev_batch"_ls), result.prevBatch);
    fromJson(jo.value("total_room_count_estimate"_ls), result.totalRoomCountEstimate);
}


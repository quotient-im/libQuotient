/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "public_rooms_response.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PublicRoomsChunk& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("aliases"), pod.aliases);
    addParam<IfNotEmpty>(jo, QStringLiteral("canonical_alias"), pod.canonicalAlias);
    addParam<IfNotEmpty>(jo, QStringLiteral("name"), pod.name);
    addParam<>(jo, QStringLiteral("num_joined_members"), pod.numJoinedMembers);
    addParam<>(jo, QStringLiteral("room_id"), pod.roomId);
    addParam<IfNotEmpty>(jo, QStringLiteral("topic"), pod.topic);
    addParam<>(jo, QStringLiteral("world_readable"), pod.worldReadable);
    addParam<>(jo, QStringLiteral("guest_can_join"), pod.guestCanJoin);
    addParam<IfNotEmpty>(jo, QStringLiteral("avatar_url"), pod.avatarUrl);
    return jo;
}

PublicRoomsChunk FromJsonObject<PublicRoomsChunk>::operator()(const QJsonObject& jo) const
{
    PublicRoomsChunk result;
    result.aliases =
        fromJson<QStringList>(jo.value("aliases"_ls));
    result.canonicalAlias =
        fromJson<QString>(jo.value("canonical_alias"_ls));
    result.name =
        fromJson<QString>(jo.value("name"_ls));
    result.numJoinedMembers =
        fromJson<qint64>(jo.value("num_joined_members"_ls));
    result.roomId =
        fromJson<QString>(jo.value("room_id"_ls));
    result.topic =
        fromJson<QString>(jo.value("topic"_ls));
    result.worldReadable =
        fromJson<bool>(jo.value("world_readable"_ls));
    result.guestCanJoin =
        fromJson<bool>(jo.value("guest_can_join"_ls));
    result.avatarUrl =
        fromJson<QString>(jo.value("avatar_url"_ls));

    return result;
}

QJsonObject QMatrixClient::toJson(const PublicRoomsResponse& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("chunk"), pod.chunk);
    addParam<IfNotEmpty>(jo, QStringLiteral("next_batch"), pod.nextBatch);
    addParam<IfNotEmpty>(jo, QStringLiteral("prev_batch"), pod.prevBatch);
    addParam<IfNotEmpty>(jo, QStringLiteral("total_room_count_estimate"), pod.totalRoomCountEstimate);
    return jo;
}

PublicRoomsResponse FromJsonObject<PublicRoomsResponse>::operator()(const QJsonObject& jo) const
{
    PublicRoomsResponse result;
    result.chunk =
        fromJson<QVector<PublicRoomsChunk>>(jo.value("chunk"_ls));
    result.nextBatch =
        fromJson<QString>(jo.value("next_batch"_ls));
    result.prevBatch =
        fromJson<QString>(jo.value("prev_batch"_ls));
    result.totalRoomCountEstimate =
        fromJson<qint64>(jo.value("total_room_count_estimate"_ls));

    return result;
}


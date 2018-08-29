/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "public_rooms_response.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PublicRoomsChunk& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("aliases"), pod.aliases);
    addParam<IfNotEmpty>(_json, QStringLiteral("canonical_alias"), pod.canonicalAlias);
    addParam<IfNotEmpty>(_json, QStringLiteral("name"), pod.name);
    addParam<>(_json, QStringLiteral("num_joined_members"), pod.numJoinedMembers);
    addParam<>(_json, QStringLiteral("room_id"), pod.roomId);
    addParam<IfNotEmpty>(_json, QStringLiteral("topic"), pod.topic);
    addParam<>(_json, QStringLiteral("world_readable"), pod.worldReadable);
    addParam<>(_json, QStringLiteral("guest_can_join"), pod.guestCanJoin);
    addParam<IfNotEmpty>(_json, QStringLiteral("avatar_url"), pod.avatarUrl);
    return _json;
}

PublicRoomsChunk FromJson<PublicRoomsChunk>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    PublicRoomsChunk result;
    result.aliases =
        fromJson<QStringList>(_json.value("aliases"_ls));
    result.canonicalAlias =
        fromJson<QString>(_json.value("canonical_alias"_ls));
    result.name =
        fromJson<QString>(_json.value("name"_ls));
    result.numJoinedMembers =
        fromJson<qint64>(_json.value("num_joined_members"_ls));
    result.roomId =
        fromJson<QString>(_json.value("room_id"_ls));
    result.topic =
        fromJson<QString>(_json.value("topic"_ls));
    result.worldReadable =
        fromJson<bool>(_json.value("world_readable"_ls));
    result.guestCanJoin =
        fromJson<bool>(_json.value("guest_can_join"_ls));
    result.avatarUrl =
        fromJson<QString>(_json.value("avatar_url"_ls));
    
    return result;
}

QJsonObject QMatrixClient::toJson(const PublicRoomsResponse& pod)
{
    QJsonObject _json;
    addParam<>(_json, QStringLiteral("chunk"), pod.chunk);
    addParam<IfNotEmpty>(_json, QStringLiteral("next_batch"), pod.nextBatch);
    addParam<IfNotEmpty>(_json, QStringLiteral("prev_batch"), pod.prevBatch);
    addParam<IfNotEmpty>(_json, QStringLiteral("total_room_count_estimate"), pod.totalRoomCountEstimate);
    return _json;
}

PublicRoomsResponse FromJson<PublicRoomsResponse>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    PublicRoomsResponse result;
    result.chunk =
        fromJson<QVector<PublicRoomsChunk>>(_json.value("chunk"_ls));
    result.nextBatch =
        fromJson<QString>(_json.value("next_batch"_ls));
    result.prevBatch =
        fromJson<QString>(_json.value("prev_batch"_ls));
    result.totalRoomCountEstimate =
        fromJson<qint64>(_json.value("total_room_count_estimate"_ls));
    
    return result;
}


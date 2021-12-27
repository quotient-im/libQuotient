/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {

struct PublicRoomsChunk {
    /// Aliases of the room. May be empty.
    QStringList aliases;

    /// The canonical alias of the room, if any.
    QString canonicalAlias;

    /// The name of the room, if any.
    QString name;

    /// The number of members joined to the room.
    int numJoinedMembers;

    /// The ID of the room.
    QString roomId;

    /// The topic of the room, if any.
    QString topic;

    /// Whether the room may be viewed by guest users without joining.
    bool worldReadable;

    /// Whether guest users may join the room and participate in it.
    /// If they can, they will be subject to ordinary power level
    /// rules like any other user.
    bool guestCanJoin;

    /// The URL for the room's avatar, if one is set.
    QUrl avatarUrl;

    /// The room's join rule. When not present, the room is assumed to
    /// be `public`. Note that rooms with `invite` join rules are not
    /// expected here, but rooms with `knock` rules are given their
    /// near-public nature.
    QString joinRule;
};

template <>
struct JsonObjectConverter<PublicRoomsChunk> {
    static void dumpTo(QJsonObject& jo, const PublicRoomsChunk& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("aliases"), pod.aliases);
        addParam<IfNotEmpty>(jo, QStringLiteral("canonical_alias"),
                             pod.canonicalAlias);
        addParam<IfNotEmpty>(jo, QStringLiteral("name"), pod.name);
        addParam<>(jo, QStringLiteral("num_joined_members"),
                   pod.numJoinedMembers);
        addParam<>(jo, QStringLiteral("room_id"), pod.roomId);
        addParam<IfNotEmpty>(jo, QStringLiteral("topic"), pod.topic);
        addParam<>(jo, QStringLiteral("world_readable"), pod.worldReadable);
        addParam<>(jo, QStringLiteral("guest_can_join"), pod.guestCanJoin);
        addParam<IfNotEmpty>(jo, QStringLiteral("avatar_url"), pod.avatarUrl);
        addParam<IfNotEmpty>(jo, QStringLiteral("join_rule"), pod.joinRule);
    }
    static void fillFrom(const QJsonObject& jo, PublicRoomsChunk& pod)
    {
        fromJson(jo.value("aliases"_ls), pod.aliases);
        fromJson(jo.value("canonical_alias"_ls), pod.canonicalAlias);
        fromJson(jo.value("name"_ls), pod.name);
        fromJson(jo.value("num_joined_members"_ls), pod.numJoinedMembers);
        fromJson(jo.value("room_id"_ls), pod.roomId);
        fromJson(jo.value("topic"_ls), pod.topic);
        fromJson(jo.value("world_readable"_ls), pod.worldReadable);
        fromJson(jo.value("guest_can_join"_ls), pod.guestCanJoin);
        fromJson(jo.value("avatar_url"_ls), pod.avatarUrl);
        fromJson(jo.value("join_rule"_ls), pod.joinRule);
    }
};

/// A list of the rooms on the server.
struct PublicRoomsResponse {
    /// A paginated chunk of public rooms.
    QVector<PublicRoomsChunk> chunk;

    /// A pagination token for the response. The absence of this token
    /// means there are no more results to fetch and the client should
    /// stop paginating.
    QString nextBatch;

    /// A pagination token that allows fetching previous results. The
    /// absence of this token means there are no results before this
    /// batch, i.e. this is the first batch.
    QString prevBatch;

    /// An estimate on the total number of public rooms, if the
    /// server has an estimate.
    Omittable<int> totalRoomCountEstimate;
};

template <>
struct JsonObjectConverter<PublicRoomsResponse> {
    static void dumpTo(QJsonObject& jo, const PublicRoomsResponse& pod)
    {
        addParam<>(jo, QStringLiteral("chunk"), pod.chunk);
        addParam<IfNotEmpty>(jo, QStringLiteral("next_batch"), pod.nextBatch);
        addParam<IfNotEmpty>(jo, QStringLiteral("prev_batch"), pod.prevBatch);
        addParam<IfNotEmpty>(jo, QStringLiteral("total_room_count_estimate"),
                             pod.totalRoomCountEstimate);
    }
    static void fillFrom(const QJsonObject& jo, PublicRoomsResponse& pod)
    {
        fromJson(jo.value("chunk"_ls), pod.chunk);
        fromJson(jo.value("next_batch"_ls), pod.nextBatch);
        fromJson(jo.value("prev_batch"_ls), pod.prevBatch);
        fromJson(jo.value("total_room_count_estimate"_ls),
                 pod.totalRoomCountEstimate);
    }
};

} // namespace Quotient

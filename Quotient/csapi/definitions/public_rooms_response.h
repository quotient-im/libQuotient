/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API PublicRoomsChunk {
    /// The number of members joined to the room.
    int numJoinedMembers;

    /// The ID of the room.
    QString roomId;

    /// Whether the room may be viewed by guest users without joining.
    bool worldReadable;

    /// Whether guest users may join the room and participate in it.
    /// If they can, they will be subject to ordinary power level
    /// rules like any other user.
    bool guestCanJoin;

    /// The canonical alias of the room, if any.
    QString canonicalAlias{};

    /// The name of the room, if any.
    QString name{};

    /// The topic of the room, if any.
    QString topic{};

    /// The URL for the room's avatar, if one is set.
    QUrl avatarUrl{};

    /// The `type` of room (from
    /// [`m.room.create`](/client-server-api/#mroomcreate)), if any.
    QString roomType{};

    /// The room's join rule. When not present, the room is assumed to
    /// be `public`. Note that rooms with `invite` join rules are not
    /// expected here, but rooms with `knock` rules are given their
    /// near-public nature.
    QString joinRule{};
};

template <>
struct JsonObjectConverter<PublicRoomsChunk> {
    static void dumpTo(QJsonObject& jo, const PublicRoomsChunk& pod)
    {
        addParam<>(jo, QStringLiteral("num_joined_members"),
                   pod.numJoinedMembers);
        addParam<>(jo, QStringLiteral("room_id"), pod.roomId);
        addParam<>(jo, QStringLiteral("world_readable"), pod.worldReadable);
        addParam<>(jo, QStringLiteral("guest_can_join"), pod.guestCanJoin);
        addParam<IfNotEmpty>(jo, QStringLiteral("canonical_alias"),
                             pod.canonicalAlias);
        addParam<IfNotEmpty>(jo, QStringLiteral("name"), pod.name);
        addParam<IfNotEmpty>(jo, QStringLiteral("topic"), pod.topic);
        addParam<IfNotEmpty>(jo, QStringLiteral("avatar_url"), pod.avatarUrl);
        addParam<IfNotEmpty>(jo, QStringLiteral("room_type"), pod.roomType);
        addParam<IfNotEmpty>(jo, QStringLiteral("join_rule"), pod.joinRule);
    }
    static void fillFrom(const QJsonObject& jo, PublicRoomsChunk& pod)
    {
        fillFromJson(jo.value("num_joined_members"_ls), pod.numJoinedMembers);
        fillFromJson(jo.value("room_id"_ls), pod.roomId);
        fillFromJson(jo.value("world_readable"_ls), pod.worldReadable);
        fillFromJson(jo.value("guest_can_join"_ls), pod.guestCanJoin);
        fillFromJson(jo.value("canonical_alias"_ls), pod.canonicalAlias);
        fillFromJson(jo.value("name"_ls), pod.name);
        fillFromJson(jo.value("topic"_ls), pod.topic);
        fillFromJson(jo.value("avatar_url"_ls), pod.avatarUrl);
        fillFromJson(jo.value("room_type"_ls), pod.roomType);
        fillFromJson(jo.value("join_rule"_ls), pod.joinRule);
    }
};

} // namespace Quotient

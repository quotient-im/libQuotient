// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API PublicRoomsChunk {
    //! The number of members joined to the room.
    int numJoinedMembers;

    //! The ID of the room.
    QString roomId;

    //! Whether the room may be viewed by guest users without joining.
    bool worldReadable;

    //! Whether guest users may join the room and participate in it.
    //! If they can, they will be subject to ordinary power level
    //! rules like any other user.
    bool guestCanJoin;

    //! The canonical alias of the room, if any.
    QString canonicalAlias{};

    //! The name of the room, if any.
    QString name{};

    //! The topic of the room, if any.
    QString topic{};

    //! The URL for the room's avatar, if one is set.
    QUrl avatarUrl{};

    //! The room's join rule. When not present, the room is assumed to
    //! be `public`. Note that rooms with `invite` join rules are not
    //! expected here, but rooms with `knock` rules are given their
    //! near-public nature.
    QString joinRule{};

    //! The `type` of room (from [`m.room.create`](/client-server-api/#mroomcreate)), if any.
    QString roomType{};
};

template <>
struct JsonObjectConverter<PublicRoomsChunk> {
    static void dumpTo(QJsonObject& jo, const PublicRoomsChunk& pod)
    {
        addParam<>(jo, "num_joined_members"_L1, pod.numJoinedMembers);
        addParam<>(jo, "room_id"_L1, pod.roomId);
        addParam<>(jo, "world_readable"_L1, pod.worldReadable);
        addParam<>(jo, "guest_can_join"_L1, pod.guestCanJoin);
        addParam<IfNotEmpty>(jo, "canonical_alias"_L1, pod.canonicalAlias);
        addParam<IfNotEmpty>(jo, "name"_L1, pod.name);
        addParam<IfNotEmpty>(jo, "topic"_L1, pod.topic);
        addParam<IfNotEmpty>(jo, "avatar_url"_L1, pod.avatarUrl);
        addParam<IfNotEmpty>(jo, "join_rule"_L1, pod.joinRule);
        addParam<IfNotEmpty>(jo, "room_type"_L1, pod.roomType);
    }
    static void fillFrom(const QJsonObject& jo, PublicRoomsChunk& pod)
    {
        fillFromJson(jo.value("num_joined_members"_L1), pod.numJoinedMembers);
        fillFromJson(jo.value("room_id"_L1), pod.roomId);
        fillFromJson(jo.value("world_readable"_L1), pod.worldReadable);
        fillFromJson(jo.value("guest_can_join"_L1), pod.guestCanJoin);
        fillFromJson(jo.value("canonical_alias"_L1), pod.canonicalAlias);
        fillFromJson(jo.value("name"_L1), pod.name);
        fillFromJson(jo.value("topic"_L1), pod.topic);
        fillFromJson(jo.value("avatar_url"_L1), pod.avatarUrl);
        fillFromJson(jo.value("join_rule"_L1), pod.joinRule);
        fillFromJson(jo.value("room_type"_L1), pod.roomType);
    }
};

} // namespace Quotient

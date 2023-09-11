// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "slidingsyncdata.h"

#include <QElapsedTimer>

using namespace Quotient;

void SlidingSyncData::parseJson(const QJsonObject &data)
{
    QElapsedTimer et;
    et.start();
    extensions = data["extensions"_ls].toObject();
    for (const auto &key : data["lists"_ls].toObject().keys()) {
        lists[key] = SlidingSyncLists::fromJson(data["lists"_ls][key].toObject());
    }
    for (const auto &key : data["rooms"_ls].toObject().keys()) {
        rooms.push_back(std::move(SlidingSyncRoom(data["rooms"_ls][key].toObject(), key)));
    }
    pos = data["pos"_ls].toString();
    qWarning() << "Sync handled in" << et.nsecsElapsed() << "nanos";
}

SlidingRoomsData SlidingSyncData::takeRoomData()
{
    return std::move(rooms);
}

SlidingSyncLists SlidingSyncLists::fromJson(const QJsonObject &data)
{
    SlidingSyncLists object;
    object.count = data["count"_ls].toInt();
    for (const auto &ops : data["ops"_ls].toArray()) {
        object.ops += SlidingSyncOps::fromJson(ops.toObject());
    }
    return object;
}

SlidingSyncOps SlidingSyncOps::fromJson(const QJsonObject &data)
{
    SlidingSyncOps object;
    object.op = data["op"_ls].toString();
    object.range = {data["range"_ls].toArray()[0].toInt(), data["range"_ls].toArray()[1].toInt()};
    for (const auto &string : data["room_ids"_ls].toArray()) {
        object.roomIds.push_back(string.toString());
    }
    return object;
}

template <typename EventsArrayT, typename StrT>
inline EventsArrayT load(const QJsonObject& batches, StrT keyName)
{
    auto json = batches[keyName];
    return fromJson<EventsArrayT>(json);
}

SlidingSyncRoom::SlidingSyncRoom(const QJsonObject &data, const QString &id)
    : id(id)
    , highlightCount(data["highlight_count"_ls].toInt())
    , initial(data["initial"_ls].toBool())
    , joinedCount(data["joined_count"_ls].toInt())
    , name(data["name"_ls].toString())
    , notificationCount(data["notification_count"_ls].toInt())
    , prevBatch(data["prev_batch"_ls].toString())
    , timeline(load<RoomEvents>(data, "timeline"_ls))
    , requiredState(load<StateEvents>(data, "required_state"_ls))
{
    // TODO invited count
}

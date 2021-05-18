// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "syncdata.h"

#include "events/eventloader.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

using namespace Quotient;

const QString SyncRoomData::UnreadCountKey =
    QStringLiteral("x-quotient.unread_count");

bool RoomSummary::isEmpty() const
{
    return !joinedMemberCount && !invitedMemberCount && !heroes;
}

bool RoomSummary::merge(const RoomSummary& other)
{
    // Using bitwise OR to prevent computation shortcut.
    return joinedMemberCount.merge(other.joinedMemberCount)
           | invitedMemberCount.merge(other.invitedMemberCount)
           | heroes.merge(other.heroes);
}

QDebug Quotient::operator<<(QDebug dbg, const RoomSummary& rs)
{
    QDebugStateSaver _(dbg);
    QStringList sl;
    if (rs.joinedMemberCount)
        sl << QStringLiteral("joined: %1").arg(*rs.joinedMemberCount);
    if (rs.invitedMemberCount)
        sl << QStringLiteral("invited: %1").arg(*rs.invitedMemberCount);
    if (rs.heroes)
        sl << QStringLiteral("heroes: [%1]").arg(rs.heroes->join(','));
    dbg.nospace().noquote() << sl.join(QStringLiteral("; "));
    return dbg;
}

void JsonObjectConverter<RoomSummary>::dumpTo(QJsonObject& jo,
                                              const RoomSummary& rs)
{
    addParam<IfNotEmpty>(jo, QStringLiteral("m.joined_member_count"),
                         rs.joinedMemberCount);
    addParam<IfNotEmpty>(jo, QStringLiteral("m.invited_member_count"),
                         rs.invitedMemberCount);
    addParam<IfNotEmpty>(jo, QStringLiteral("m.heroes"), rs.heroes);
}

void JsonObjectConverter<RoomSummary>::fillFrom(const QJsonObject& jo,
                                                RoomSummary& rs)
{
    fromJson(jo["m.joined_member_count"_ls], rs.joinedMemberCount);
    fromJson(jo["m.invited_member_count"_ls], rs.invitedMemberCount);
    fromJson(jo["m.heroes"_ls], rs.heroes);
}

template <typename EventsArrayT, typename StrT>
inline EventsArrayT load(const QJsonObject& batches, StrT keyName)
{
    return fromJson<EventsArrayT>(batches[keyName].toObject().value("events"_ls));
}

QDebug Quotient::operator<<(QDebug dbg, const DevicesList& devicesList)
{
    QDebugStateSaver _(dbg);
    QStringList sl;
    if (!devicesList.changed.isEmpty())
        sl << QStringLiteral("changed: %1").arg(devicesList.changed.join(", "));
    if (!devicesList.left.isEmpty())
        sl << QStringLiteral("left %1").arg(devicesList.left.join(", "));
    dbg.nospace().noquote() << sl.join(QStringLiteral("; "));
    return dbg;
}

void JsonObjectConverter<DevicesList>::dumpTo(QJsonObject& jo,
                                              const DevicesList& rs)
{
    addParam<IfNotEmpty>(jo, QStringLiteral("changed"),
                         rs.changed);
    addParam<IfNotEmpty>(jo, QStringLiteral("left"),
                         rs.left);
}

void JsonObjectConverter<DevicesList>::fillFrom(const QJsonObject& jo,
                                                DevicesList& rs)
{
    fromJson(jo["changed"_ls], rs.changed);
    fromJson(jo["left"_ls], rs.left);
}

SyncRoomData::SyncRoomData(const QString& roomId_, JoinState joinState_,
                           const QJsonObject& room_)
    : roomId(roomId_)
    , joinState(joinState_)
    , summary(fromJson<RoomSummary>(room_["summary"_ls]))
    , state(load<StateEvents>(room_, joinState == JoinState::Invite
                                         ? "invite_state"_ls
                                         : "state"_ls))
{
    switch (joinState) {
    case JoinState::Join:
        ephemeral = load<Events>(room_, "ephemeral"_ls);
        [[fallthrough]];
    case JoinState::Leave: {
        accountData = load<Events>(room_, "account_data"_ls);
        timeline = load<RoomEvents>(room_, "timeline"_ls);
        const auto timelineJson = room_.value("timeline"_ls).toObject();
        timelineLimited = timelineJson.value("limited"_ls).toBool();
        timelinePrevBatch = timelineJson.value("prev_batch"_ls).toString();

        break;
    }
    default: /* nothing on top of state */;
    }

    const auto unreadJson = room_.value("unread_notifications"_ls).toObject();
    fromJson(unreadJson.value(UnreadCountKey), unreadCount);
    fromJson(unreadJson.value("highlight_count"_ls), highlightCount);
    fromJson(unreadJson.value("notification_count"_ls), notificationCount);
    if (highlightCount.has_value() || notificationCount.has_value())
        qCDebug(SYNCJOB) << "Room" << roomId_
                         << "has highlights:" << *highlightCount
                         << "and notifications:" << *notificationCount;
}

SyncData::SyncData(const QString& cacheFileName)
{
    QFileInfo cacheFileInfo { cacheFileName };
    auto json = loadJson(cacheFileName);
    auto requiredVersion = std::get<0>(cacheVersion());
    auto actualVersion =
        json.value("cache_version"_ls).toObject().value("major"_ls).toInt();
    if (actualVersion == requiredVersion)
        parseJson(json, cacheFileInfo.absolutePath() + '/');
    else
        qCWarning(MAIN) << "Major version of the cache file is" << actualVersion
                        << "but" << requiredVersion
                        << "is required; discarding the cache";
}

SyncDataList&& SyncData::takeRoomData() { return move(roomData); }

QString SyncData::fileNameForRoom(QString roomId)
{
    roomId.replace(':', '_');
    return roomId + ".json";
}

Events&& SyncData::takePresenceData() { return std::move(presenceData); }

Events&& SyncData::takeAccountData() { return std::move(accountData); }

Events&& SyncData::takeToDeviceEvents() { return std::move(toDeviceEvents); }

DevicesList&& SyncData::takeDevicesList() { return std::move(devicesList); }

QJsonObject SyncData::loadJson(const QString& fileName)
{
    QFile roomFile { fileName };
    if (!roomFile.exists()) {
        qCWarning(MAIN) << "No state cache file" << fileName;
        return {};
    }
    if (!roomFile.open(QIODevice::ReadOnly)) {
        qCWarning(MAIN) << "Failed to open state cache file"
                        << roomFile.fileName();
        return {};
    }
    auto data = roomFile.readAll();

    const auto json = data.startsWith('{')
                          ? QJsonDocument::fromJson(data).object()
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                          : QCborValue::fromCbor(data).toJsonValue().toObject()
#else
                          : QJsonDocument::fromBinaryData(data).object()
#endif
        ;
    if (json.isEmpty()) {
        qCWarning(MAIN) << "State cache in" << fileName
                        << "is broken or empty, discarding";
    }
    return json;
}

void SyncData::parseJson(const QJsonObject& json, const QString& baseDir)
{
    QElapsedTimer et;
    et.start();

    nextBatch_ = json.value("next_batch"_ls).toString();
    presenceData = load<Events>(json, "presence"_ls);
    accountData = load<Events>(json, "account_data"_ls);
    toDeviceEvents = load<Events>(json, "to_device"_ls);

    fromJson(json.value("device_one_time_keys_count"_ls),
             deviceOneTimeKeysCount_);

    if(json.contains("device_lists")) {
        fromJson(json.value("device_lists"), devicesList);
    }

    auto rooms = json.value("rooms"_ls).toObject();
    auto totalRooms = 0;
    auto totalEvents = 0;
    // The first comparison shortcuts the loop when not all states are there
    // in the response (anything except "join" is only occasional, and "join"
    // intentionally comes first in the enum).
    for (size_t i = 0;
         static_cast<int>(i) < rooms.size() && i < JoinStateStrings.size(); ++i)
    {
        // This assumes that MemberState values go over powers of 2: 1,2,4,...
        const auto joinState = JoinState(1U << i);
        const auto rs = rooms.value(JoinStateStrings[i]).toObject();
        // We have a Qt container on the right and an STL one on the left
        roomData.reserve(roomData.size() + static_cast<size_t>(rs.size()));
        for (auto roomIt = rs.begin(); roomIt != rs.end(); ++roomIt) {
            auto roomJson =
                roomIt->isObject()
                    ? roomIt->toObject()
                    : loadJson(baseDir + fileNameForRoom(roomIt.key()));
            if (roomJson.isEmpty()) {
                unresolvedRoomIds.push_back(roomIt.key());
                continue;
            }
            roomData.emplace_back(roomIt.key(), joinState, roomJson);
            const auto& r = roomData.back();
            totalEvents += r.state.size() + r.ephemeral.size()
                           + r.accountData.size() + r.timeline.size();
        }
        totalRooms += rs.size();
    }
    if (!unresolvedRoomIds.empty())
        qCWarning(MAIN) << "Unresolved rooms:" << unresolvedRoomIds.join(',');
    if (totalRooms > 9 || et.nsecsElapsed() >= profilerMinNsecs())
        qCDebug(PROFILER) << "*** SyncData::parseJson(): batch with"
                          << totalRooms << "room(s)," << totalEvents
                          << "event(s) in" << et;
}

/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "syncdata.h"

#include "events/eventloader.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

using namespace QMatrixClient;

const QString SyncRoomData::UnreadCountKey =
        QStringLiteral("x-qmatrixclient.unread_count");

bool RoomSummary::isEmpty() const
{
    return joinedMemberCount.omitted() && invitedMemberCount.omitted()
            && heroes.omitted();
}

bool RoomSummary::merge(const RoomSummary& other)
{
    // Using bitwise OR to prevent computation shortcut.
    return joinedMemberCount.merge(other.joinedMemberCount)
            | invitedMemberCount.merge(other.invitedMemberCount)
            | heroes.merge(other.heroes);
}

QDebug QMatrixClient::operator<<(QDebug dbg, const RoomSummary& rs)
{
    QDebugStateSaver _(dbg);
    QStringList sl;
    if (!rs.joinedMemberCount.omitted())
        sl << QStringLiteral("joined: %1").arg(rs.joinedMemberCount.value());
    if (!rs.invitedMemberCount.omitted())
        sl << QStringLiteral("invited: %1").arg(rs.invitedMemberCount.value());
    if (!rs.heroes.omitted())
        sl << QStringLiteral("heroes: [%1]").arg(rs.heroes.value().join(','));
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
    fromJson(jo["m.heroes"], rs.heroes);
}

template <typename EventsArrayT, typename StrT>
inline EventsArrayT load(const QJsonObject& batches, StrT keyName)
{
    return fromJson<EventsArrayT>(
            batches[keyName].toObject().value("events"_ls));
}

SyncRoomData::SyncRoomData(const QString& roomId_, JoinState joinState_,
                           const QJsonObject& room_)
    : roomId(roomId_),
      joinState(joinState_),
      summary(fromJson<RoomSummary>(room_["summary"])),
      state(load<StateEvents>(room_,
                              joinState == JoinState::Invite ? "invite_state"_ls
                                                             : "state"_ls))
{
    switch (joinState) {
    case JoinState::Join:
        ephemeral = load<Events>(room_, "ephemeral"_ls);
        FALLTHROUGH;
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
    unreadCount = unreadJson.value(UnreadCountKey).toInt(-2);
    highlightCount = unreadJson.value("highlight_count"_ls).toInt();
    notificationCount = unreadJson.value("notification_count"_ls).toInt();
    if (highlightCount > 0 || notificationCount > 0)
        qCDebug(SYNCJOB) << "Room" << roomId_
                         << "has highlights:" << highlightCount
                         << "and notifications:" << notificationCount;
}

SyncData::SyncData(const QString& cacheFileName)
{
    QFileInfo cacheFileInfo { cacheFileName };
    auto json = loadJson(cacheFileName);
    auto requiredVersion = std::get<0>(cacheVersion());
    auto actualVersion =
            json.value("cache_version").toObject().value("major").toInt();
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

    const auto json =
            (data.startsWith('{') ? QJsonDocument::fromJson(data)
                                  : QJsonDocument::fromBinaryData(data))
                    .object();
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

    auto rooms = json.value("rooms"_ls).toObject();
    JoinStates::Int ii = 1; // ii is used to make a JoinState value
    auto totalRooms = 0;
    auto totalEvents = 0;
    for (size_t i = 0; i < JoinStateStrings.size(); ++i, ii <<= 1) {
        const auto rs = rooms.value(JoinStateStrings[i]).toObject();
        // We have a Qt container on the right and an STL one on the left
        roomData.reserve(static_cast<size_t>(rs.size()));
        for (auto roomIt = rs.begin(); roomIt != rs.end(); ++roomIt) {
            auto roomJson = roomIt->isObject()
                    ? roomIt->toObject()
                    : loadJson(baseDir + fileNameForRoom(roomIt.key()));
            if (roomJson.isEmpty()) {
                unresolvedRoomIds.push_back(roomIt.key());
                continue;
            }
            roomData.emplace_back(roomIt.key(), JoinState(ii), roomJson);
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

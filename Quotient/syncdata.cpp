// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "syncdata.h"

#include "logging_categories_p.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <private/qjson_p.h>

using namespace Quotient;

bool RoomSummary::isEmpty() const
{
    return !joinedMemberCount && !invitedMemberCount && !heroes;
}

QDebug Quotient::operator<<(QDebug dbg, const RoomSummary& rs)
{
    QDebugStateSaver _(dbg);
    dbg.nospace().noquote();
    auto joiner = "";
    if (rs.joinedMemberCount) {
        dbg << "joined: " << *rs.joinedMemberCount;
        joiner = ", ";
    }
    if (rs.invitedMemberCount) {
        dbg << joiner << "invited: " << *rs.invitedMemberCount;
        joiner = ", ";
    }
    if (rs.heroes)
        dbg << joiner << "heroes: " << rs.heroes->join(u',');
    return dbg;
}

void JsonObjectConverter<RoomSummary>::dumpTo(QJsonObject& jo,
                                              const RoomSummary& rs)
{
    addParam<IfNotEmpty>(jo, "m.joined_member_count"_L1, rs.joinedMemberCount);
    addParam<IfNotEmpty>(jo, "m.invited_member_count"_L1, rs.invitedMemberCount);
    addParam<IfNotEmpty>(jo, "m.heroes"_L1, rs.heroes);
}

void JsonObjectConverter<RoomSummary>::fillFrom(const QJsonObject& jo,
                                                RoomSummary& rs)
{
    fromJson(jo["m.joined_member_count"_L1], rs.joinedMemberCount);
    fromJson(jo["m.invited_member_count"_L1], rs.invitedMemberCount);
    fromJson(jo["m.heroes"_L1], rs.heroes);
}

template <typename EventsArrayT, typename StrT>
inline EventsArrayT load(const QJsonObject& batches, StrT keyName)
{
    return fromJson<EventsArrayT>(batches[keyName].toObject().value("events"_L1));
}

SyncRoomData::SyncRoomData(QString roomId_, JoinState joinState, const QJsonObject& roomJson)
    : roomId(std::move(roomId_))
    , joinState(joinState)
    , summary(fromJson<RoomSummary>(roomJson["summary"_L1]))
    , state(load<StateEvents>(roomJson,
                              joinState == JoinState::Invite ? "invite_state"_L1 : "state"_L1))
{
    switch (joinState) {
    case JoinState::Join:
        ephemeral = load<Events>(roomJson, "ephemeral"_L1);
        [[fallthrough]];
    case JoinState::Leave: {
        accountData = load<Events>(roomJson, "account_data"_L1);
        timeline = load<RoomEvents>(roomJson, "timeline"_L1);
        const auto timelineJson = roomJson.value("timeline"_L1).toObject();
        timelineLimited = timelineJson.value("limited"_L1).toBool();
        timelinePrevBatch = timelineJson.value("prev_batch"_L1).toString();

        break;
    }
    default: /* nothing on top of state */;
    }

    const auto unreadJson = roomJson.value(UnreadNotificationsKey).toObject();

    fromJson(unreadJson.value(PartiallyReadCountKey), partiallyReadCount);
    if (!partiallyReadCount.has_value())
        fromJson(unreadJson.value("x-quotient.unread_count"_L1),
                 partiallyReadCount);

    fromJson(roomJson.value(NewUnreadCountKey), unreadCount);
    if (!unreadCount.has_value())
        fromJson(unreadJson.value("notification_count"_L1), unreadCount);
    fromJson(unreadJson.value(HighlightCountKey), highlightCount);
}

QDebug Quotient::operator<<(QDebug dbg, const DevicesList& devicesList)
{
    QDebugStateSaver _(dbg);
    dbg.nospace().noquote();
    auto joiner = "";
    if (!devicesList.changed.isEmpty()) {
        dbg << "changed: " << devicesList.changed.join(", "_L1);
        joiner = "; ";
    }
    if (!devicesList.left.isEmpty())
        dbg << joiner << "left " << devicesList.left.join(", "_L1);
    return dbg;
}

void JsonObjectConverter<DevicesList>::dumpTo(QJsonObject& jo,
                                              const DevicesList& rs)
{
    addParam<IfNotEmpty>(jo, u"changed"_s, rs.changed);
    addParam<IfNotEmpty>(jo, u"left"_s, rs.left);
}

void JsonObjectConverter<DevicesList>::fillFrom(const QJsonObject& jo,
                                                DevicesList& rs)
{
    fromJson(jo["changed"_L1], rs.changed);
    fromJson(jo["left"_L1], rs.left);
}

namespace {
QJsonObject loadJson(const QString& fileName)
{
    QFile cacheFile { fileName };
    if (!cacheFile.exists()) {
        qCWarning(MAIN) << "No state cache file" << fileName;
        return {};
    }
    if (!cacheFile.open(QIODevice::ReadOnly)) {
        qCWarning(MAIN) << "Failed to open state cache file"
                        << cacheFile.fileName();
        return {};
    }
    auto data = cacheFile.readAll();

    const auto json = data.startsWith('{')
                          ? QJsonDocument::fromJson(data).object()
                          : QJsonPrivate::Value::fromTrustedCbor(QCborValue::fromCbor(data)).toObject();
    if (json.isEmpty()) {
        qCWarning(MAIN) << "State cache in" << fileName
                        << "is broken or empty, discarding";
    }
    return json;
}
}

SyncData::SyncData(const QString& cacheFileName)
{
    auto json = loadJson(cacheFileName);
    auto requiredVersion = MajorCacheVersion;
    auto actualVersion =
        json.value("cache_version"_L1).toObject().value("major"_L1).toInt();
    if (actualVersion == requiredVersion)
        parseJson(json, QFileInfo(cacheFileName).absolutePath() + u'/');
    else
        qCWarning(MAIN) << "Major version of the cache file is" << actualVersion
                        << "but" << requiredVersion
                        << "is required; discarding the cache";
}

SyncDataList SyncData::takeRoomData() { return std::move(roomData); }

QString SyncData::fileNameForRoom(QString roomId)
{
    roomId.replace(u':', u'_');
    return roomId + ".json"_L1;
}

Events SyncData::takePresenceData() { return std::move(presenceData); }

Events SyncData::takeAccountData() { return std::move(accountData); }

Events SyncData::takeToDeviceEvents() { return std::move(toDeviceEvents); }

std::pair<int, int> SyncData::cacheVersion()
{
    return { MajorCacheVersion, 3 };
}

DevicesList SyncData::takeDevicesList() { return std::move(devicesList); }

// FIXME, 0.9: baseDir -> cacheDir
void SyncData::parseJson(const QJsonObject& json, const QString& baseDir)
{
    QElapsedTimer et;
    et.start();

    nextBatch_ = json.value("next_batch"_L1).toString();
    presenceData = load<Events>(json, "presence"_L1);
    accountData = load<Events>(json, "account_data"_L1);
    toDeviceEvents = load<Events>(json, "to_device"_L1);

    fromJson(json.value("device_one_time_keys_count"_L1),
             deviceOneTimeKeysCount_);

    if(json.contains("device_lists"_L1)) {
        fromJson(json.value("device_lists"_L1), devicesList);
    }

    auto rooms = json.value("rooms"_L1).toObject();
    qsizetype totalRooms = 0;
    size_t totalEvents = 0;
    for (size_t i = 0; i < JoinStateStrings.size(); ++i) {
        // This assumes that MemberState values go over powers of 2: 1,2,4,...
        const auto joinState = JoinState(1U << i);
        const auto rs = rooms.value(JoinStateStrings[i]).toObject();
        // We have a Qt container on the right and an STL one on the left
        roomData.reserve(roomData.size() + static_cast<size_t>(rs.size()));
        for (auto roomIt = rs.begin(); roomIt != rs.end(); ++roomIt) {
            QJsonObject roomJson;
            // Normally (i.e. in a /sync response) the received JSON is
            // self-contained; but the local cache stores state for each room in
            // its own file, loaded below
            if (Q_UNLIKELY(!baseDir.isEmpty())) {
                roomJson = loadJson(
                    baseDir
                    + (roomIt->isObject() // lib 0.8.1.2 onwards = cache 11.3 onwards
                           ? roomIt->toObject().value("$ref"_L1).toString()
                           : fileNameForRoom(roomIt.key()))); // lib pre-0.8.1.2 = cache pre-11.3
                if (roomJson.isEmpty()) {
                    unresolvedRoomIds.push_back(roomIt.key());
                    continue;
                }
            } else // When loading from /sync response, everything is inline
                roomJson = roomIt->toObject();

            roomData.emplace_back(roomIt.key(), joinState, roomJson);
            const auto& r = roomData.back();
            totalEvents += r.state.size() + r.ephemeral.size()
                           + r.accountData.size() + r.timeline.size();
        }
        totalRooms += rs.size();
    }
    if (!unresolvedRoomIds.empty())
        qCWarning(MAIN) << "Unresolved rooms:" << unresolvedRoomIds.join(u',');
    if (totalRooms > 9 || et.nsecsElapsed() >= ProfilerMinNsecs)
        qCDebug(PROFILER) << "*** SyncData::parseJson(): batch with"
                          << totalRooms << "room(s)," << totalEvents
                          << "event(s) in" << et;
}

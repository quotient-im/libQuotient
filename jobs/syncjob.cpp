/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "syncjob.h"

#include <QtCore/QElapsedTimer>

using namespace QMatrixClient;

static size_t jobId = 0;

SyncJob::SyncJob(const ConnectionData* connection, const QString& since,
                 const QString& filter, int timeout, const QString& presence)
    : BaseJob(connection, HttpVerb::Get, QString("SyncJob-%1").arg(++jobId),
              "_matrix/client/r0/sync")
    , d(new SyncData)
{
    setLoggingCategory(SYNCJOB);
    QUrlQuery query;
    if( !filter.isEmpty() )
        query.addQueryItem("filter", filter);
    if( !presence.isEmpty() )
        query.addQueryItem("set_presence", presence);
    if( timeout >= 0 )
        query.addQueryItem("timeout", QString::number(timeout));
    if( !since.isEmpty() )
        query.addQueryItem("since", since);
    setRequestQuery(query);

    setMaxRetries(std::numeric_limits<int>::max());
}

SyncJob::~SyncJob()
{
    delete d;
}

QString SyncData::nextBatch() const
{
    return nextBatch_;
}

SyncDataList&& SyncData::takeRoomData()
{
    return std::move(roomData);
}

BaseJob::Status SyncJob::parseJson(const QJsonDocument& data)
{
    return d->parseJson(data);
}

BaseJob::Status SyncData::parseJson(const QJsonDocument &data) {
    QElapsedTimer et; et.start();
    QJsonObject json = data.object();
    nextBatch_ = json.value("next_batch").toString();
    // TODO: presence
    // TODO: account_data
    QJsonObject rooms = json.value("rooms").toObject();

    const struct { QString jsonKey; JoinState enumVal; } roomStates[]
    {
        { "join", JoinState::Join },
        { "invite", JoinState::Invite },
        { "leave", JoinState::Leave }
    };
    for (auto roomState: roomStates)
    {
        const QJsonObject rs = rooms.value(roomState.jsonKey).toObject();
        // We have a Qt container on the right and an STL one on the left
        roomData.reserve(static_cast<size_t>(rs.size()));
        for( auto rkey: rs.keys() )
            roomData.emplace_back(rkey, roomState.enumVal, rs[rkey].toObject());
    }
    qCDebug(PROFILER) << "*** SyncData::parseJson():" << et.elapsed() << "ms";
    return BaseJob::Success;
}

SyncRoomData::SyncRoomData(const QString& roomId_, JoinState joinState_,
                           const QJsonObject& room_)
    : roomId(roomId_)
    , joinState(joinState_)
    , state("state")
    , timeline("timeline")
    , ephemeral("ephemeral")
    , accountData("account_data")
    , inviteState("invite_state")
{
    switch (joinState) {
        case JoinState::Invite:
            inviteState.fromJson(room_);
            break;
        case JoinState::Join:
            state.fromJson(room_);
            timeline.fromJson(room_);
            ephemeral.fromJson(room_);
            accountData.fromJson(room_);
            break;
        case JoinState::Leave:
            state.fromJson(room_);
            timeline.fromJson(room_);
            break;
    default:
        qCWarning(SYNCJOB) << "SyncRoomData: Unknown JoinState value, ignoring:" << int(joinState);
    }

    QJsonObject timeline = room_.value("timeline").toObject();
    timelineLimited = timeline.value("limited").toBool();
    timelinePrevBatch = timeline.value("prev_batch").toString();

    QJsonObject unread = room_.value("unread_notifications").toObject();
    highlightCount = unread.value("highlight_count").toInt();
    notificationCount = unread.value("notification_count").toInt();
    qCDebug(SYNCJOB) << "Highlights: " << highlightCount << " Notifications:" << notificationCount;
}

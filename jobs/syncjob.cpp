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

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>
#include <QtCore/QDebug>

#include "../room.h"
#include "../connectiondata.h"
#include "../events/event.h"

using namespace QMatrixClient;

class SyncJob::Private
{
    public:
        QString since;
        QString filter;
        bool fullState;
        QString presence;
        int timeout;
        QString nextBatch;

        QList<SyncRoomData> roomData;
};

static size_t jobId = 0;

SyncJob::SyncJob(ConnectionData* connection, QString since)
    : BaseJob(connection, JobHttpType::GetJob, QString("SyncJob-%1").arg(++jobId))
    , d(new Private)
{
    d->since = since;
    d->fullState = false;
    d->timeout = -1;
}

SyncJob::~SyncJob()
{
    delete d;
}

void SyncJob::setFilter(QString filter)
{
    d->filter = filter;
}

void SyncJob::setFullState(bool full)
{
    d->fullState = full;
}

void SyncJob::setPresence(QString presence)
{
    d->presence = presence;
}

void SyncJob::setTimeout(int timeout)
{
    d->timeout = timeout;
}

QString SyncJob::nextBatch() const
{
    return d->nextBatch;
}

QList<SyncRoomData> SyncJob::roomData() const
{
    return d->roomData;
}

QString SyncJob::apiPath() const
{
    return "_matrix/client/r0/sync";
}

QUrlQuery SyncJob::query() const
{
    QUrlQuery query;
    if( !d->filter.isEmpty() )
        query.addQueryItem("filter", d->filter);
    if( d->fullState )
        query.addQueryItem("full_state", "true");
    if( !d->presence.isEmpty() )
        query.addQueryItem("set_presence", d->presence);
    if( d->timeout >= 0 )
        query.addQueryItem("timeout", QString::number(d->timeout));
    if( !d->since.isEmpty() )
        query.addQueryItem("since", d->since);
    return query;
}

BaseJob::Status SyncJob::parseJson(const QJsonDocument& data)
{
    QJsonObject json = data.object();
    d->nextBatch = json.value("next_batch").toString();
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
        d->roomData.reserve(rs.size());
        for( auto r = rs.begin(); r != rs.end(); ++r )
        {
            d->roomData.push_back({r.key(), r.value().toObject(), roomState.enumVal});
        }
    }

    return Success;
}

void SyncRoomData::EventList::fromJson(const QJsonObject& roomContents)
{
    auto l = eventListFromJson(roomContents[jsonKey].toObject()["events"].toArray());
    swap(l);
}

SyncRoomData::SyncRoomData(QString roomId_, const QJsonObject& room_, JoinState joinState_)
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
        qWarning() << "SyncRoomData: Unknown JoinState value, ignoring:" << int(joinState);
    }

    QJsonObject timeline = room_.value("timeline").toObject();
    timelineLimited = timeline.value("limited").toBool();
    timelinePrevBatch = timeline.value("prev_batch").toString();

    QJsonObject unread = room_.value("unread_notifications").toObject();
    highlightCount = unread.value("highlight_count").toInt();
    notificationCount = unread.value("notification_count").toInt();
    qDebug() << "Highlights: " << highlightCount << " Notifications:" << notificationCount;
}

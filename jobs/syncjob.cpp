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

SyncJob::SyncJob(ConnectionData* connection, QString since)
    : BaseJob(connection, JobHttpType::GetJob, "SyncJob")
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

QString SyncJob::apiPath()
{
    return "_matrix/client/r0/sync";
}

QUrlQuery SyncJob::query()
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

void SyncJob::parseJson(const QJsonDocument& data)
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
        for( auto r = rs.begin(); r != rs.end(); ++r )
        {
            d->roomData.append({r.key(), r.value().toObject(), roomState.enumVal});
        }
    }

    emitResult();
}

SyncRoomData::SyncRoomData(QString roomId_, const QJsonObject& room_, JoinState joinState_)
    : roomId(roomId_), joinState(joinState_)
{
    const struct { QString jsonKey; QList<Event *> *evList; } eventLists[]
    {
        { "state", &state },
        { "timeline", &timeline },
        { "ephemeral", &ephemeral },
        { "account_data", &accountData }
    };

    for (auto e: eventLists)
    {
        QJsonArray array = room_.value(e.jsonKey).toObject().value("events").toArray();
        for( QJsonValue val: array )
        {
            if ( Event* event = Event::fromJson(val.toObject()) )
                e.evList->append(event);
        }
    }

    QJsonObject timeline = room_.value("timeline").toObject();
    timelineLimited = timeline.value("limited").toBool();
    timelinePrevBatch = timeline.value("prev_batch").toString();

    QJsonObject unread = room_.value("unread_notifications").toObject();
    highlightCount = unread.value("highlight_count").toInt();
    notificationCount = unread.value("notification_count").toInt();
    qDebug() << "Highlights: " << highlightCount << " Notifications:" << notificationCount;
}

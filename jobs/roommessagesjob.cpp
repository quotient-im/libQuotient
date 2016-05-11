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

#include "roommessagesjob.h"
#include "../room.h"
#include "../events/event.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

using namespace QMatrixClient;

class RoomMessagesJob::Private
{
    public:
        Room* room;
        QString from;
        FetchDirectory dir;
        int limit;

        QList<Event*> events;
};

RoomMessagesJob::RoomMessagesJob(ConnectionData* data, Room* room, QString from, FetchDirectory dir, int limit)
    : SimpleJob(data, JobHttpType::GetJob, "RoomMessagesJob")
    , d(new Private{room, from, dir, limit, {} })
    , end("end", *this)
{ }

RoomMessagesJob::~RoomMessagesJob()
{
    delete d;
}

QList<Event*> RoomMessagesJob::events()
{
    return d->events;
}

QString RoomMessagesJob::apiPath()
{
    return QString("/_matrix/client/r0/rooms/%1/messages").arg(d->room->id());
}

QUrlQuery RoomMessagesJob::query()
{
    QUrlQuery query;
    query.setQueryItems({
          { "from", d->from }
        , { "limit", QString::number(d->limit) }
        , { "dir", d->dir == FetchDirectory::Backwards ? "b" : "f" }
    });
    return query;
}

void RoomMessagesJob::parseJson(const QJsonDocument& data)
{
    QJsonObject obj = data.object();
    QJsonArray chunk = obj.value("chunk").toArray();
    for( const QJsonValue& val: chunk )
    {
        Event* event = Event::fromJson(val.toObject());
        d->events.append(event);
    }
    SimpleJob::parseJson(data);
}

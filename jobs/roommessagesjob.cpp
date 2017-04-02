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
#include "../util.h"

#include <QtCore/QJsonArray>

using namespace QMatrixClient;

class RoomMessagesJob::Private
{
    public:
        Private() {}

        Owning<Events> events;
        QString end;
};

RoomMessagesJob::RoomMessagesJob(ConnectionData* data, QString roomId,
                                 QString from, int limit, FetchDirection dir)
    : BaseJob(data, HttpVerb::Get, "RoomMessagesJob",
              QString("/_matrix/client/r0/rooms/%1/messages").arg(roomId),
              Query(
                { { "from", from }
                , { "dir", dir == FetchDirection::Backward ? "b" : "f" }
                , { "limit", QString::number(limit) }
                }))
    , d(new Private)
{
    qDebug() << "Room messages query:" << query().toString(QUrl::PrettyDecoded);
}

RoomMessagesJob::~RoomMessagesJob()
{
    delete d;
}

Events RoomMessagesJob::releaseEvents()
{
    return d->events.release();
}

QString RoomMessagesJob::end()
{
    return d->end;
}

BaseJob::Status RoomMessagesJob::parseJson(const QJsonDocument& data)
{
    QJsonObject obj = data.object();
    d->events.assign(eventsFromJson(obj.value("chunk").toArray()));
    d->end = obj.value("end").toString();
    return Success;
}

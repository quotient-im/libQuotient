/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
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

#include "postmessagejob.h"
#include "../room.h"
#include "../connectiondata.h"

#include <QtNetwork/QNetworkReply>

using namespace QMatrixClient;

class PostMessageJob::Private
{
    public:
        Private() {}

        QString type;
        QString message;
        Room* room;
};

PostMessageJob::PostMessageJob(ConnectionData* connection, Room* room, QString type, QString message)
    : BaseJob(connection, JobHttpType::PostJob, "PostMessageJob")
    , d(new Private)
{
    d->type = type;
    d->message = message;
    d->room = room;
}

PostMessageJob::~PostMessageJob()
{
    delete d;
}

QString PostMessageJob::apiPath() const
{
    return QString("_matrix/client/r0/rooms/%1/send/m.room.message").arg(d->room->id());
}

QJsonObject PostMessageJob::data() const
{
    QJsonObject json;
    json.insert("msgtype", d->type);
    json.insert("body", d->message);
    return json;
}

BaseJob::Status PostMessageJob::parseJson(const QJsonDocument& data)
{
    if( data.object().contains("event_id") )
        return Success;

    qDebug() << data;
    return { UserDefinedError, "No event_id in the JSON response" };
}

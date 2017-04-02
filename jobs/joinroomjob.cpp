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

#include "joinroomjob.h"

#include <QtNetwork/QNetworkReply>

#include "../connectiondata.h"

using namespace QMatrixClient;

class JoinRoomJob::Private
{
    public:
        QString roomId;
};

JoinRoomJob::JoinRoomJob(ConnectionData* data, QString roomAlias)
    : BaseJob(data, HttpVerb::Post, "JoinRoomJob",
              QString("_matrix/client/r0/join/%1").arg(roomAlias))
    , d(new Private)
{
}

JoinRoomJob::~JoinRoomJob()
{
    delete d;
}

QString JoinRoomJob::roomId()
{
    return d->roomId;
}

BaseJob::Status JoinRoomJob::parseJson(const QJsonDocument& data)
{
    QJsonObject json = data.object();
    if( json.contains("room_id") )
    {
        d->roomId = json.value("room_id").toString();
        return Success;
    }

    qDebug() << data;
    return { UserDefinedError, "No room_id in the JSON response" };
}

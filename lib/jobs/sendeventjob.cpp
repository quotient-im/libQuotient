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

#include "sendeventjob.h"

#include "events/roommessageevent.h"

using namespace QMatrixClient;

SendEventJob::SendEventJob(const QString& roomId, const QString& type,
                           const QString& plainText)
    : SendEventJob(roomId, RoomMessageEvent(plainText, type))
{ }

void SendEventJob::beforeStart(const ConnectionData* connData)
{
    BaseJob::beforeStart(connData);
    setApiEndpoint(apiEndpoint() + connData->generateTxnId());
}

BaseJob::Status SendEventJob::parseJson(const QJsonDocument& data)
{
    _eventId = data.object().value("event_id"_ls).toString();
    if (!_eventId.isEmpty())
        return Success;

    qCDebug(JOBS) << data;
    return { UserDefinedError, "No event_id in the JSON response" };
}


/******************************************************************************
* Copyright (C) 2019 QMatrixClient project
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

#include "roomcreateevent.h"

using namespace QMatrixClient;

RoomCreateDetails::RoomCreateDetails(const QJsonObject& json)
    : federated(fromJson<bool>(json["m.federate"_ls]))
    , version(fromJson<QString>(json["room_version"_ls]))
{
    const auto predecessorJson = json["predecessor"_ls].toObject();
    if (!predecessorJson.isEmpty())
    {
        fromJson(predecessorJson["room_id"_ls], predRoomId);
        fromJson(predecessorJson["event_id"_ls], predEventId);
    }
}

std::pair<QString, QString> RoomCreateEvent::predecessor() const
{
    return { content().predRoomId, content().predEventId };
}

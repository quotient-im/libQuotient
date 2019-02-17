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

bool RoomCreateEvent::isFederated() const
{
    return fromJson<bool>(contentJson()["m.federate"_ls]);
}

QString RoomCreateEvent::version() const
{
    return fromJson<QString>(contentJson()["room_version"_ls]);
}

RoomCreateEvent::Predecessor RoomCreateEvent::predecessor() const
{
    const auto predJson = contentJson()["predecessor"_ls].toObject();
    return {
        fromJson<QString>(predJson["room_id"_ls]),
        fromJson<QString>(predJson["event_id"_ls])
    };
}

bool RoomCreateEvent::isUpgrade() const
{
    return contentJson().contains("predecessor"_ls);
}

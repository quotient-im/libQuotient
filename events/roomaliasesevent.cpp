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

// Example of a RoomAliases Event:
//
// {
//     "age":3758857346,
//     "content":{
//         "aliases":["#freenode_#testest376:matrix.org"]
//     },
//     "event_id":"$1439989428122drFjY:matrix.org",
//     "origin_server_ts":1439989428910,
//     "replaces_state":"$143613875199223YYPrN:matrix.org",
//     "room_id":"!UoqtanuuSGTMvNRfDG:matrix.org",
//     "state_key":"matrix.org",
//     "type":"m.room.aliases",
//     "user_id":"@appservice-irc:matrix.org"
// }

#include "roomaliasesevent.h"

using namespace QMatrixClient;

RoomAliasesEvent::RoomAliasesEvent(const QJsonObject& obj)
    : RoomEvent(Type::RoomAliases, obj)
    , _aliases(toStringList(contentJson()["aliases"]))
{ }


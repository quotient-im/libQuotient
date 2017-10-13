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

#include "roommemberevent.h"

#include "logging.h"

using namespace QMatrixClient;

static const auto membershipStrings =
    { "invite", "join", "knock", "leave", "ban" };

RoomMemberEvent::RoomMemberEvent(const QJsonObject& obj)
    : RoomEvent(Type::RoomMember, obj), _userId(obj["state_key"].toString())
{
    const auto contentObj = contentJson();
    _displayName = contentObj["displayname"].toString();
    _avatarUrl = contentObj["avatar_url"].toString();
    QString membershipString = contentObj["membership"].toString();
    for (auto it = membershipStrings.begin(); it != membershipStrings.end(); ++it)
        if (membershipString == *it)
        {
            _membership = MembershipType(it - membershipStrings.begin());
            return;
        }
    qCWarning(EVENTS) << "Unknown MembershipType: " << membershipString;
}

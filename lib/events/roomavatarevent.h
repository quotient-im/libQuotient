/******************************************************************************
 * Copyright (C) 2017 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include "eventcontent.h"
#include "stateevent.h"

namespace QMatrixClient {
class RoomAvatarEvent : public StateEvent<EventContent::ImageContent> {
    // It's a bit of an overkill to use a full-fledged ImageContent
    // because in reality m.room.avatar usually only has a single URL,
    // without a thumbnail. But The Spec says there be thumbnails, and
    // we follow The Spec.
public:
    DEFINE_EVENT_TYPEID("m.room.avatar", RoomAvatarEvent)
    explicit RoomAvatarEvent(const QJsonObject& obj) : StateEvent(typeId(), obj)
    {}
    QUrl url() const { return content().url; }
};
REGISTER_EVENT_TYPE(RoomAvatarEvent)
DEFINE_EVENTTYPE_ALIAS(RoomAvatar, RoomAvatarEvent)
} // namespace QMatrixClient

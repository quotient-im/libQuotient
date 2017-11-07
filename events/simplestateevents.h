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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "event.h"

#include "eventcontent.h"

namespace QMatrixClient
{
#define DECLARE_SIMPLE_STATE_EVENT(_Name, _TypeId, _EnumType, _ContentType, _ContentKey) \
    class _Name \
        : public StateEvent<EventContent::SimpleContent<_ContentType>> \
    { \
        public: \
            static constexpr const char* TypeId = _TypeId; \
            explicit _Name(const QJsonObject& obj) \
                : StateEvent(_EnumType, obj, #_ContentKey) \
            { } \
            template <typename T> \
            explicit _Name(T&& value) \
                : StateEvent(_EnumType, #_ContentKey, \
                             std::forward<T>(value)) \
            { } \
            _ContentType _ContentKey() const { return content().value; } \
    };

    DECLARE_SIMPLE_STATE_EVENT(RoomNameEvent, "m.room.name",
                               Event::Type::RoomName, QString, name)
    DECLARE_SIMPLE_STATE_EVENT(RoomAliasesEvent, "m.room.aliases",
                               Event::Type::RoomAliases, QStringList, aliases)
    DECLARE_SIMPLE_STATE_EVENT(RoomCanonicalAliasEvent, "m.room.canonical_alias",
                               Event::Type::RoomCanonicalAlias, QString, alias)
    DECLARE_SIMPLE_STATE_EVENT(RoomTopicEvent, "m.room.topic",
                               Event::Type::RoomTopic, QString, topic)
    DECLARE_SIMPLE_STATE_EVENT(EncryptionEvent, "m.room.encryption",
                               Event::Type::RoomEncryption, QString, algorithm)
} // namespace QMatrixClient

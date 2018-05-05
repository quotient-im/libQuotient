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

#pragma once

#include "event.h"

#include "eventcontent.h"

#include <QtCore/QUrl>

namespace QMatrixClient
{
    class MemberEventContent: public EventContent::Base
    {
        public:
            enum MembershipType : size_t { Invite = 0, Join, Knock, Leave, Ban,
                                           Undefined };

            explicit MemberEventContent(MembershipType mt = MembershipType::Join)
                : membership(mt)
            { }
            explicit MemberEventContent(const QJsonObject& json);

            MembershipType membership;
            bool isDirect = false;
            QString displayName;
            QUrl avatarUrl;

        protected:
            void fillJson(QJsonObject* o) const override;
    };

    using MembershipType = MemberEventContent::MembershipType;

    class RoomMemberEvent: public StateEvent<MemberEventContent>
    {
            Q_GADGET
        public:
            static constexpr const char* typeId() { return "m.room.member"; }

            using MembershipType = MemberEventContent::MembershipType;

            explicit RoomMemberEvent(Type type, const QJsonObject& obj)
                : StateEvent(type, obj)
            { }
            RoomMemberEvent(MemberEventContent&& c)
                : StateEvent(Type::RoomMember, c)
            { }
            explicit RoomMemberEvent(const QJsonObject& obj)
                : RoomMemberEvent(Type::RoomMember, obj)
            { }

            MembershipType membership() const  { return content().membership; }
            QString userId() const
            { return originalJsonObject().value("state_key").toString(); }
            bool isDirect() const { return content().isDirect; }
            QString displayName() const { return content().displayName; }
            QUrl avatarUrl() const      { return content().avatarUrl; }

        private:
            REGISTER_ENUM(MembershipType)
    };
}  // namespace QMatrixClient

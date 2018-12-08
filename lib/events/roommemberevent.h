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

#include "stateevent.h"
#include "eventcontent.h"

namespace QMatrixClient
{
    class MemberEventContent: public EventContent::Base
    {
        public:
            enum MembershipType : size_t { Invite = 0, Join, Knock, Leave, Ban,
                                           Undefined };

            explicit MemberEventContent(MembershipType mt = Join)
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
            DEFINE_EVENT_TYPEID("m.room.member", RoomMemberEvent)

            using MembershipType = MemberEventContent::MembershipType;

            explicit RoomMemberEvent(const QJsonObject& obj)
                : StateEvent(typeId(), obj)
            { }
            RoomMemberEvent(MemberEventContent&& c)
                : StateEvent(typeId(), matrixTypeId(), c)
            { }

            // This is a special constructor enabling RoomMemberEvent to be
            // a base class for more specific member events.
            RoomMemberEvent(Type type, const QJsonObject& fullJson)
                : StateEvent(type, fullJson)
            { }

            MembershipType membership() const  { return content().membership; }
            QString userId() const
                { return fullJson()["state_key"_ls].toString(); }
            bool isDirect() const { return content().isDirect; }
            QString displayName() const { return content().displayName; }
            QUrl avatarUrl() const      { return content().avatarUrl; }
            bool isInvite() const;
            bool isJoin() const;
            bool isLeave() const;
            bool isRename() const;
            bool isAvatarUpdate() const;

        private:
            REGISTER_ENUM(MembershipType)
    };
    REGISTER_EVENT_TYPE(RoomMemberEvent)
    DEFINE_EVENTTYPE_ALIAS(RoomMember, RoomMemberEvent)
}  // namespace QMatrixClient

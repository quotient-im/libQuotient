// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2017 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Karol Kosek <krkkx@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "eventcontent.h"
#include "stateevent.h"

namespace Quotient {
class MemberEventContent : public EventContent::Base {
public:
    enum MembershipType : unsigned char {
        Invite = 0,
        Join,
        Knock,
        Leave,
        Ban,
        Undefined
    };

    explicit MemberEventContent(MembershipType mt = Join) : membership(mt) {}
    explicit MemberEventContent(const QJsonObject& json);

    MembershipType membership;
    bool isDirect = false;
    Omittable<QString> displayName;
    Omittable<QUrl> avatarUrl;
    QString reason;

protected:
    void fillJson(QJsonObject* o) const override;
};

using MembershipType = MemberEventContent::MembershipType;

class RoomMemberEvent : public StateEvent<MemberEventContent> {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.room.member", RoomMemberEvent)

    using MembershipType = MemberEventContent::MembershipType;
    Q_ENUM(MembershipType)

    explicit RoomMemberEvent(const QJsonObject& obj) : StateEvent(typeId(), obj)
    {}
    template <typename... ArgTs>
    RoomMemberEvent(const QString& userId, ArgTs&&... contentArgs)
        : StateEvent(typeId(), matrixTypeId(), userId,
                     std::forward<ArgTs>(contentArgs)...)
    {}

    /// A special constructor to create unknown RoomMemberEvents
    /**
     * This is needed in order to use RoomMemberEvent as a "base event
     * class" in cases like GetMembersByRoomJob when RoomMemberEvents
     * (rather than RoomEvents or StateEvents) are resolved from JSON.
     * For such cases loadEvent<> requires an underlying class to be
     * constructible with unknownTypeId() instead of its genuine id.
     * Don't use it directly.
     * \sa GetMembersByRoomJob, loadEvent, unknownTypeId
     */
    RoomMemberEvent(Type type, const QJsonObject& fullJson)
        : StateEvent(type, fullJson)
    {}

    MembershipType membership() const { return content().membership; }
    QString userId() const { return stateKey(); }
    bool isDirect() const { return content().isDirect; }
    Omittable<QString> newDisplayName() const { return content().displayName; }
    Omittable<QUrl> newAvatarUrl() const { return content().avatarUrl; }
    [[deprecated("Use newDisplayName() instead")]] QString displayName() const
    {
        return newDisplayName().value_or(QString());
    }
    [[deprecated("Use newAvatarUrl() instead")]] QUrl avatarUrl() const
    {
        return newAvatarUrl().value_or(QUrl());
    }
    QString reason() const { return content().reason; }
    bool changesMembership() const;
    bool isBan() const;
    bool isUnban() const;
    bool isInvite() const;
    bool isRejectedInvite() const;
    bool isJoin() const;
    bool isLeave() const;
    bool isRename() const;
    bool isAvatarUpdate() const;
};

template <>
class EventFactory<RoomMemberEvent> {
public:
    static event_ptr_tt<RoomMemberEvent> make(const QJsonObject& json,
                                              const QString&)
    {
        return makeEvent<RoomMemberEvent>(json);
    }
};

REGISTER_EVENT_TYPE(RoomMemberEvent)
} // namespace Quotient

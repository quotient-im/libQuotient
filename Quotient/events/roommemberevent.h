// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2017 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Karol Kosek <krkkx@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"
#include <Quotient/quotient_common.h>

namespace Quotient {
class QUOTIENT_API MemberEventContent {
public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    QUO_IMPLICIT MemberEventContent(Membership ms) : membership(ms) {}
    explicit MemberEventContent(const QJsonObject& json);
    QJsonObject toJson() const;

    Membership membership;
    /// (Only for invites) Whether the invite is to a direct chat
    bool isDirect = false;
    Omittable<QString> displayName;
    Omittable<QUrl> avatarUrl;
    QString reason;
};

class QUOTIENT_API RoomMemberEvent
    : public KeyedStateEventBase<RoomMemberEvent, MemberEventContent> {
    Q_GADGET
public:
    QUO_EVENT(RoomMemberEvent, "m.room.member")

    static bool isValid(const QJsonObject& fullJson)
    {
        return !fullJson[StateKeyKey].toString().isEmpty();
    }

    using KeyedStateEventBase::KeyedStateEventBase;

    Membership membership() const { return content().membership; }
    QString userId() const { return stateKey(); }
    bool isDirect() const { return content().isDirect; }
    Omittable<QString> newDisplayName() const { return content().displayName; }
    Omittable<QUrl> newAvatarUrl() const { return content().avatarUrl; }
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
} // namespace Quotient

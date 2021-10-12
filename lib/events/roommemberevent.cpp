// SPDX-FileCopyrightText: 2017 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-FileCopyrightText: 2019 Karol Kosek <krkkx@protonmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roommemberevent.h"

#include "converters.h"
#include "logging.h"

#include <QtCore/QtAlgorithms>

namespace Quotient {
template <>
struct JsonConverter<Membership> {
    static Membership load(const QJsonValue& jv)
    {
        const auto& ms = jv.toString();
        if (ms.isEmpty())
        {
            qCWarning(EVENTS) << "Empty member state:" << ms;
            return Membership::Invalid;
        }
        const auto it =
            std::find(MembershipStrings.begin(), MembershipStrings.end(), ms);
        if (it != MembershipStrings.end())
            return Membership(1U << (it - MembershipStrings.begin()));

        qCWarning(EVENTS) << "Unknown Membership value: " << ms;
        return Membership::Invalid;
    }
};
} // namespace Quotient

using namespace Quotient;

MemberEventContent::MemberEventContent(const QJsonObject& json)
    : membership(fromJson<Membership>(json["membership"_ls]))
    , isDirect(json["is_direct"_ls].toBool())
    , displayName(fromJson<Omittable<QString>>(json["displayname"_ls]))
    , avatarUrl(fromJson<Omittable<QString>>(json["avatar_url"_ls]))
    , reason(json["reason"_ls].toString())
{
    if (displayName)
        displayName = sanitized(*displayName);
}

void MemberEventContent::fillJson(QJsonObject* o) const
{
    Q_ASSERT(o);
    if (membership != Membership::Invalid)
        o->insert(
            QStringLiteral("membership"),
            MembershipStrings[qCountTrailingZeroBits(
                                  std::underlying_type_t<Membership>(membership))
                              + 1]);
    if (displayName)
        o->insert(QStringLiteral("displayname"), *displayName);
    if (avatarUrl && avatarUrl->isValid())
        o->insert(QStringLiteral("avatar_url"), avatarUrl->toString());
    if (!reason.isEmpty())
        o->insert(QStringLiteral("reason"), reason);
}

bool RoomMemberEvent::changesMembership() const
{
    return !prevContent() || prevContent()->membership != membership();
}

bool RoomMemberEvent::isInvite() const
{
    return membership() == Membership::Invite && changesMembership();
}

bool RoomMemberEvent::isRejectedInvite() const
{
    return membership() == Membership::Leave && prevContent()
           && prevContent()->membership == Membership::Invite;
}

bool RoomMemberEvent::isJoin() const
{
    return membership() == Membership::Join && changesMembership();
}

bool RoomMemberEvent::isLeave() const
{
    return membership() == Membership::Leave && prevContent()
           && prevContent()->membership != membership()
           && prevContent()->membership != Membership::Ban
           && prevContent()->membership != Membership::Invite;
}

bool RoomMemberEvent::isBan() const
{
    return membership() == Membership::Ban && changesMembership();
}

bool RoomMemberEvent::isUnban() const
{
    return membership() == Membership::Leave && prevContent()
           && prevContent()->membership == Membership::Ban;
}

bool RoomMemberEvent::isRename() const
{
    return prevContent() && prevContent()->displayName
               ? newDisplayName() != *prevContent()->displayName
               : newDisplayName().has_value();
}

bool RoomMemberEvent::isAvatarUpdate() const
{
    return prevContent() && prevContent()->avatarUrl
               ? newAvatarUrl() != *prevContent()->avatarUrl
               : newAvatarUrl().has_value();
}

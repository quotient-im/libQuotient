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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "roommemberevent.h"

#include "converters.h"
#include "logging.h"

#include <array>

static const std::array<QString, 5> membershipStrings = {
    { QStringLiteral("invite"), QStringLiteral("join"), QStringLiteral("knock"),
      QStringLiteral("leave"), QStringLiteral("ban") }
};

namespace Quotient {
template <>
struct JsonConverter<MembershipType> {
    static MembershipType load(const QJsonValue& jv)
    {
        const auto& membershipString = jv.toString();
        for (auto it = membershipStrings.begin(); it != membershipStrings.end();
             ++it)
            if (membershipString == *it)
                return MembershipType(it - membershipStrings.begin());

        qCWarning(EVENTS) << "Unknown MembershipType: " << membershipString;
        return MembershipType::Undefined;
    }
};
} // namespace Quotient

using namespace Quotient;

MemberEventContent::MemberEventContent(const QJsonObject& json)
    : membership(fromJson<MembershipType>(json["membership"_ls]))
    , isDirect(json["is_direct"_ls].toBool())
    , displayName(fromJson<Omittable<QString>>(json["displayname"_ls]))
    , avatarUrl(fromJson<Omittable<QUrl>>(json["avatar_url"_ls]))
    , reason(json["reason"_ls].toString())
{
    if (displayName)
        displayName = sanitized(*displayName);
}

void MemberEventContent::fillJson(QJsonObject* o) const
{
    Q_ASSERT(o);
    Q_ASSERT_X(membership != MembershipType::Undefined, __FUNCTION__,
               "The key 'membership' must be explicit in MemberEventContent");
    if (membership != MembershipType::Undefined)
        o->insert(QStringLiteral("membership"), membershipStrings[membership]);
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
    return membership() == MembershipType::Invite && changesMembership();
}

bool RoomMemberEvent::isRejectedInvite() const
{
    return membership() == MembershipType::Leave && prevContent()
           && prevContent()->membership == MembershipType::Invite;
}

bool RoomMemberEvent::isJoin() const
{
    return membership() == MembershipType::Join && changesMembership();
}

bool RoomMemberEvent::isLeave() const
{
    return membership() == MembershipType::Leave && prevContent()
           && prevContent()->membership != membership()
           && prevContent()->membership != MembershipType::Ban
           && prevContent()->membership != MembershipType::Invite;
}

bool RoomMemberEvent::isBan() const
{
    return membership() == MembershipType::Ban && changesMembership();
}

bool RoomMemberEvent::isUnban() const
{
    return membership() == MembershipType::Leave && prevContent()
           && prevContent()->membership == MembershipType::Ban;
}

bool RoomMemberEvent::isRename() const
{
    auto prevName = prevContent() && prevContent()->displayName
                        ? *prevContent()->displayName
                        : QString();
    return newDisplayName() != prevName;
}

bool RoomMemberEvent::isAvatarUpdate() const
{
    auto prevAvatarUrl = prevContent() && prevContent()->avatarUrl
                             ? *prevContent()->avatarUrl
                             : QUrl();
    return newAvatarUrl() != prevAvatarUrl;
}

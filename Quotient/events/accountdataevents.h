// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

#include "../csapi/definitions/tag.h"

namespace Quotient {
constexpr inline auto FavouriteTag = "m.favourite"_L1;
constexpr inline auto LowPriorityTag = "m.lowpriority"_L1;
constexpr inline auto ServerNoticeTag = "m.server_notice"_L1;

using TagRecord [[deprecated("Use Tag from csapi/definitions/tag.h instead")]] = Tag;

inline std::partial_ordering operator<=>( //
    const Tag& lhs, const Tag& rhs) // clazy:exclude=function-args-by-value
{
    // Per The Spec, rooms with no order should be after those with order,
    // against std::optional<>::operator<=>() convention.
    return (lhs.order && !rhs.order)   ? std::partial_ordering::less
           : (!lhs.order && rhs.order) ? std::partial_ordering::greater
                                       : *lhs.order <=> *rhs.order;
}

using TagsMap = QHash<QString, Tag>;

DEFINE_SIMPLE_EVENT(TagEvent, Event, "m.tag", TagsMap, tags, "tags")
DEFINE_SIMPLE_EVENT(ReadMarkerEvent, Event, "m.fully_read", QString,
                    eventId, "event_id")
DEFINE_SIMPLE_EVENT(IgnoredUsersEvent, Event, "m.ignored_user_list",
                    QSet<QString>, ignoredUsers, "ignored_users")
} // namespace Quotient

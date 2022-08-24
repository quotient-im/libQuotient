// SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"
#include "util.h"

namespace Quotient {
constexpr auto FavouriteTag [[maybe_unused]] = "m.favourite"_ls;
constexpr auto LowPriorityTag [[maybe_unused]] = "m.lowpriority"_ls;
constexpr auto ServerNoticeTag [[maybe_unused]] = "m.server_notice"_ls;

struct TagRecord {
    Omittable<float> order = none;
};

inline bool operator<(TagRecord lhs, TagRecord rhs)
{
    // Per The Spec, rooms with no order should be after those with order,
    // against std::optional<>::operator<() convention.
    return lhs.order && (!rhs.order || *lhs.order < *rhs.order);
}

template <>
struct JsonObjectConverter<TagRecord> {
    static void fillFrom(const QJsonObject& jo, TagRecord& rec)
    {
        // Parse a float both from JSON double and JSON string because
        // the library previously used to use strings to store order.
        const auto orderJv = jo.value("order"_ls);
        if (orderJv.isDouble())
            rec.order = fromJson<float>(orderJv);
        if (orderJv.isString()) {
            bool ok = false;
            rec.order = orderJv.toString().toFloat(&ok);
            if (!ok)
                rec.order = none;
        }
    }
    static void dumpTo(QJsonObject& jo, TagRecord rec)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("order"), rec.order);
    }
};

using TagsMap = QHash<QString, TagRecord>;

DEFINE_SIMPLE_EVENT(TagEvent, Event, "m.tag", TagsMap, tags)
DEFINE_SIMPLE_EVENT(ReadMarkerEventImpl, Event, "m.fully_read", QString, eventId)
class ReadMarkerEvent : public ReadMarkerEventImpl {
public:
    using ReadMarkerEventImpl::ReadMarkerEventImpl;
    [[deprecated("Use ReadMarkerEvent::eventId() instead")]]
    QString event_id() const { return eventId(); }
};
DEFINE_SIMPLE_EVENT(IgnoredUsersEvent, Event, "m.ignored_user_list", QSet<QString>,
                    ignored_users)
} // namespace Quotient

/******************************************************************************
 * SPDX-FileCopyrightText: 2018 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "event.h"
#include "eventcontent.h"

namespace Quotient {
constexpr const char* FavouriteTag = "m.favourite";
constexpr const char* LowPriorityTag = "m.lowpriority";
constexpr const char* ServerNoticeTag = "m.server_notice";

struct TagRecord {
    using order_type = Omittable<float>;

    order_type order;

    TagRecord(order_type order = none) : order(std::move(order)) {}

    bool operator<(const TagRecord& other) const
    {
        // Per The Spec, rooms with no order should be after those with order,
        // against optional<>::operator<() convention.
        return order && (!other.order || *order < *other.order);
    }
};

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
            bool ok;
            rec.order = orderJv.toString().toFloat(&ok);
            if (!ok)
                rec.order = none;
        }
    }
    static void dumpTo(QJsonObject& jo, const TagRecord& rec)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("order"), rec.order);
    }
};

using TagsMap = QHash<QString, TagRecord>;

#define DEFINE_SIMPLE_EVENT(_Name, _TypeId, _ContentType, _ContentKey)       \
    class _Name : public Event {                                             \
    public:                                                                  \
        using content_type = _ContentType;                                   \
        DEFINE_EVENT_TYPEID(_TypeId, _Name)                                  \
        explicit _Name(QJsonObject obj) : Event(typeId(), std::move(obj)) {} \
        explicit _Name(_ContentType content)                                 \
            : Event(typeId(), matrixTypeId(),                                \
                    QJsonObject { { QStringLiteral(#_ContentKey),            \
                                    toJson(std::move(content)) } })          \
        {}                                                                   \
        auto _ContentKey() const                                             \
        {                                                                    \
            return content<content_type>(#_ContentKey##_ls);                 \
        }                                                                    \
    };                                                                       \
    REGISTER_EVENT_TYPE(_Name)                                               \
    // End of macro

DEFINE_SIMPLE_EVENT(TagEvent, "m.tag", TagsMap, tags)
DEFINE_SIMPLE_EVENT(ReadMarkerEvent, "m.fully_read", QString, event_id)
DEFINE_SIMPLE_EVENT(IgnoredUsersEvent, "m.ignored_user_list", QSet<QString>,
                    ignored_users)
} // namespace Quotient

#include <utility>

/******************************************************************************
 * Copyright (C) 2018 Kitsune Ral <kitsune-ral@users.sf.net>
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
    static constexpr const char* FavouriteTag = "m.favourite";
    static constexpr const char* LowPriorityTag = "m.lowpriority";

    struct TagRecord
    {
        TagRecord (QString order = {}) : order(std::move(order)) { }
        explicit TagRecord(const QJsonValue& jv)
            : order(jv.toObject().value("order").toString())
        { }

        QString order;

        bool operator==(const TagRecord& other) const
            { return order == other.order; }
        bool operator!=(const TagRecord& other) const
            { return !operator==(other); }
    };

    inline QJsonValue toJson(const TagRecord& rec)
    {
        return QJsonObject {{ QStringLiteral("order"), rec.order }};
    }

    using TagsMap = QHash<QString, TagRecord>;

#define DEFINE_SIMPLE_EVENT(_Name, _TypeId, _EnumType, _ContentType, _ContentKey) \
    class _Name : public Event \
    { \
        public: \
            static constexpr const char* TypeId = _TypeId; \
            static const char* typeId() { return TypeId; } \
            explicit _Name(const QJsonObject& obj) \
                : Event((_EnumType), obj) \
                , _content(contentJson(), QStringLiteral(#_ContentKey)) \
            { } \
            template <typename... Ts> \
            explicit _Name(Ts&&... contentArgs) \
                : Event(_EnumType) \
                , _content(QStringLiteral(#_ContentKey), \
                           std::forward<Ts>(contentArgs)...) \
            { } \
            const _ContentType& _ContentKey() const { return _content.value; } \
            QJsonObject toJson() const { return _content.toJson(); } \
        protected: \
            EventContent::SimpleContent<_ContentType> _content; \
    };

    DEFINE_SIMPLE_EVENT(TagEvent, "m.tag", EventType::Tag, TagsMap, tags)
    DEFINE_SIMPLE_EVENT(ReadMarkerEvent, "m.fully_read", EventType::ReadMarker,
                        QString, event_id)
}

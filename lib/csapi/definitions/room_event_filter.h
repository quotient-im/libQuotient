/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "csapi/definitions/event_filter.h"
#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct RoomEventFilter : EventFilter
    {
        /// A list of room IDs to exclude. If this list is absent then no rooms are excluded. A matching room will be excluded even if it is listed in the ``'rooms'`` filter.
        QStringList notRooms;
        /// A list of room IDs to include. If this list is absent then all rooms are included.
        QStringList rooms;
        /// If ``true``, includes only events with a ``url`` key in their content. If ``false``, excludes those events. If omitted, ``url`` key is not considered for filtering.
        Omittable<bool> containsUrl;
    };

    QJsonObject toJson(const RoomEventFilter& pod);

    template <> struct FromJsonObject<RoomEventFilter>
    {
        RoomEventFilter operator()(const QJsonObject& jo) const;
    };

} // namespace QMatrixClient

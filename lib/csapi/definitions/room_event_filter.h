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
        /// If ``true``, includes only events with a ``url`` key in their content. If ``false``, excludes those events. Defaults to ``false``.
        bool containsUrl;
        /// If ``true``, the only ``m.room.member`` events returned in the ``state`` section of the ``/sync`` response are those which are definitely necessary for a client to display the ``sender`` of the timeline events in that response.  If ``false``, ``m.room.member`` events are not filtered.  By default, servers should suppress duplicate redundant lazy-loaded ``m.room.member`` events from being sent to a given client across multiple calls to ``/sync``, given that most clients cache membership events (see include_redundant_members to change this behaviour).
        bool lazyLoadMembers;
        /// If ``true``, the ``state`` section of the ``/sync`` response will always contain the ``m.room.member`` events required to display the ``sender`` of the timeline events in that response, assuming ``lazy_load_members`` is enabled. This means that redundant duplicate member events may be returned across multiple calls to ``/sync``. This is useful for naive clients who never track membership data. If ``false``, duplicate ``m.room.member`` events may be suppressed by the server across multiple calls to ``/sync``.  If ``lazy_load_members`` is ``false`` this field is ignored.
        bool includeRedundantMembers;
    };

    QJsonObject toJson(const RoomEventFilter& pod);

    template <> struct FromJsonObject<RoomEventFilter>
    {
        RoomEventFilter operator()(const QJsonObject& jo) const;
    };

} // namespace QMatrixClient

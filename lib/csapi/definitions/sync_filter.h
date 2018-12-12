/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "csapi/definitions/room_event_filter.h"
#include "converters.h"
#include "csapi/definitions/event_filter.h"

namespace QMatrixClient
{
    // Data structures

    /// The state events to include for rooms.
    struct StateFilter : RoomEventFilter
    {
        /// If ``true``, the only ``m.room.member`` events returned in
        /// the ``state`` section of the ``/sync`` response are those
        /// which are definitely necessary for a client to display
        /// the ``sender`` of the timeline events in that response.
        /// If ``false``, ``m.room.member`` events are not filtered.
        /// By default, servers should suppress duplicate redundant
        /// lazy-loaded ``m.room.member`` events from being sent to a given
        /// client across multiple calls to ``/sync``, given that most clients
        /// cache membership events (see ``include_redundant_members``
        /// to change this behaviour).
        Omittable<bool> lazyLoadMembers;
        /// If ``true``, the ``state`` section of the ``/sync`` response will
        /// always contain the ``m.room.member`` events required to display
        /// the ``sender`` of the timeline events in that response, assuming
        /// ``lazy_load_members`` is enabled. This means that redundant
        /// duplicate member events may be returned across multiple calls to
        /// ``/sync``. This is useful for naive clients who never track
        /// membership data. If ``false``, duplicate ``m.room.member`` events
        /// may be suppressed by the server across multiple calls to ``/sync``.
        /// If ``lazy_load_members`` is ``false`` this field is ignored.
        Omittable<bool> includeRedundantMembers;
    };
    template <> struct JsonObjectConverter<StateFilter>
    {
        static void dumpTo(QJsonObject& jo, const StateFilter& pod);
        static void fillFrom(const QJsonObject& jo, StateFilter& pod);
    };

    /// Filters to be applied to room data.
    struct RoomFilter
    {
        /// A list of room IDs to exclude. If this list is absent then no rooms are excluded. A matching room will be excluded even if it is listed in the ``'rooms'`` filter. This filter is applied before the filters in ``ephemeral``, ``state``, ``timeline`` or ``account_data``
        QStringList notRooms;
        /// A list of room IDs to include. If this list is absent then all rooms are included. This filter is applied before the filters in ``ephemeral``, ``state``, ``timeline`` or ``account_data``
        QStringList rooms;
        /// The events that aren't recorded in the room history, e.g. typing and receipts, to include for rooms.
        Omittable<RoomEventFilter> ephemeral;
        /// Include rooms that the user has left in the sync, default false
        Omittable<bool> includeLeave;
        /// The state events to include for rooms.
        Omittable<StateFilter> state;
        /// The message and state update events to include for rooms.
        Omittable<RoomEventFilter> timeline;
        /// The per user account data to include for rooms.
        Omittable<RoomEventFilter> accountData;
    };
    template <> struct JsonObjectConverter<RoomFilter>
    {
        static void dumpTo(QJsonObject& jo, const RoomFilter& pod);
        static void fillFrom(const QJsonObject& jo, RoomFilter& pod);
    };

    struct Filter
    {
        /// List of event fields to include. If this list is absent then all fields are included. The entries may include '.' charaters to indicate sub-fields. So ['content.body'] will include the 'body' field of the 'content' object. A literal '.' character in a field name may be escaped using a '\\'. A server may include more fields than were requested.
        QStringList eventFields;
        /// The format to use for events. 'client' will return the events in a format suitable for clients. 'federation' will return the raw event as receieved over federation. The default is 'client'.
        QString eventFormat;
        /// The presence updates to include.
        Omittable<EventFilter> presence;
        /// The user account data that isn't associated with rooms to include.
        Omittable<EventFilter> accountData;
        /// Filters to be applied to room data.
        Omittable<RoomFilter> room;
    };
    template <> struct JsonObjectConverter<Filter>
    {
        static void dumpTo(QJsonObject& jo, const Filter& pod);
        static void fillFrom(const QJsonObject& jo, Filter& pod);
    };

} // namespace QMatrixClient

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
        bool includeLeave;
        /// The state events to include for rooms.
        Omittable<RoomEventFilter> state;
        /// The message and state update events to include for rooms.
        Omittable<RoomEventFilter> timeline;
        /// The per user account data to include for rooms.
        Omittable<RoomEventFilter> accountData;
    };

    QJsonObject toJson(const RoomFilter& pod);

    template <> struct FromJsonObject<RoomFilter>
    {
        RoomFilter operator()(const QJsonObject& jo) const;
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

    QJsonObject toJson(const Filter& pod);

    template <> struct FromJsonObject<Filter>
    {
        Filter operator()(const QJsonObject& jo) const;
    };

} // namespace QMatrixClient

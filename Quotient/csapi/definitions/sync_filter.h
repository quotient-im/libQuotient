/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/csapi/definitions/event_filter.h>
#include <Quotient/csapi/definitions/room_event_filter.h>

#include <Quotient/converters.h>

namespace Quotient {
/// Filters to be applied to room data.
struct QUOTIENT_API RoomFilter {
    /// A list of room IDs to exclude. If this list is absent then no rooms are
    /// excluded. A matching room will be excluded even if it is listed in the
    /// `'rooms'` filter. This filter is applied before the filters in
    /// `ephemeral`, `state`, `timeline` or `account_data`
    QStringList notRooms{};

    /// A list of room IDs to include. If this list is absent then all rooms are
    /// included. This filter is applied before the filters in `ephemeral`,
    /// `state`, `timeline` or `account_data`
    QStringList rooms{};

    /// The ephemeral events to include for rooms. These are the events that
    /// appear in the `ephemeral` property in the `/sync` response.
    RoomEventFilter ephemeral{};

    /// Include rooms that the user has left in the sync, default false
    Omittable<bool> includeLeave{};

    /// The state events to include for rooms.
    RoomEventFilter state{};

    /// The message and state update events to include for rooms.
    RoomEventFilter timeline{};

    /// The per user account data to include for rooms.
    RoomEventFilter accountData{};
};

template <>
struct JsonObjectConverter<RoomFilter> {
    static void dumpTo(QJsonObject& jo, const RoomFilter& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("not_rooms"), pod.notRooms);
        addParam<IfNotEmpty>(jo, QStringLiteral("rooms"), pod.rooms);
        addParam<IfNotEmpty>(jo, QStringLiteral("ephemeral"), pod.ephemeral);
        addParam<IfNotEmpty>(jo, QStringLiteral("include_leave"),
                             pod.includeLeave);
        addParam<IfNotEmpty>(jo, QStringLiteral("state"), pod.state);
        addParam<IfNotEmpty>(jo, QStringLiteral("timeline"), pod.timeline);
        addParam<IfNotEmpty>(jo, QStringLiteral("account_data"),
                             pod.accountData);
    }
    static void fillFrom(const QJsonObject& jo, RoomFilter& pod)
    {
        fillFromJson(jo.value("not_rooms"_ls), pod.notRooms);
        fillFromJson(jo.value("rooms"_ls), pod.rooms);
        fillFromJson(jo.value("ephemeral"_ls), pod.ephemeral);
        fillFromJson(jo.value("include_leave"_ls), pod.includeLeave);
        fillFromJson(jo.value("state"_ls), pod.state);
        fillFromJson(jo.value("timeline"_ls), pod.timeline);
        fillFromJson(jo.value("account_data"_ls), pod.accountData);
    }
};

struct QUOTIENT_API Filter {
    /// List of event fields to include. If this list is absent then all fields
    /// are included. The entries are [dot-separated paths for each
    /// property](/appendices#dot-separated-property-paths) to include. So
    /// ['content.body'] will include the 'body' field of the 'content' object.
    /// A server may include more fields than were requested.
    QStringList eventFields{};

    /// The format to use for events. 'client' will return the events in a
    /// format suitable for clients. 'federation' will return the raw event as
    /// received over federation. The default is 'client'.
    QString eventFormat{};

    /// The presence updates to include.
    EventFilter presence{};

    /// The user account data that isn't associated with rooms to include.
    EventFilter accountData{};

    /// Filters to be applied to room data.
    RoomFilter room{};
};

template <>
struct JsonObjectConverter<Filter> {
    static void dumpTo(QJsonObject& jo, const Filter& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("event_fields"),
                             pod.eventFields);
        addParam<IfNotEmpty>(jo, QStringLiteral("event_format"),
                             pod.eventFormat);
        addParam<IfNotEmpty>(jo, QStringLiteral("presence"), pod.presence);
        addParam<IfNotEmpty>(jo, QStringLiteral("account_data"),
                             pod.accountData);
        addParam<IfNotEmpty>(jo, QStringLiteral("room"), pod.room);
    }
    static void fillFrom(const QJsonObject& jo, Filter& pod)
    {
        fillFromJson(jo.value("event_fields"_ls), pod.eventFields);
        fillFromJson(jo.value("event_format"_ls), pod.eventFormat);
        fillFromJson(jo.value("presence"_ls), pod.presence);
        fillFromJson(jo.value("account_data"_ls), pod.accountData);
        fillFromJson(jo.value("room"_ls), pod.room);
    }
};

} // namespace Quotient

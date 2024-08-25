// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/event_filter.h>

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API RoomEventFilter : EventFilter {
    //! If `true`, enables per-[thread](/client-server-api/#threading) notification
    //! counts. Only applies to the `/sync` endpoint. Defaults to `false`.
    std::optional<bool> unreadThreadNotifications{};

    //! If `true`, enables lazy-loading of membership events. See
    //! [Lazy-loading room members](/client-server-api/#lazy-loading-room-members)
    //! for more information. Defaults to `false`.
    std::optional<bool> lazyLoadMembers{};

    //! If `true`, sends all membership events for all events, even if they have already
    //! been sent to the client. Does not
    //! apply unless `lazy_load_members` is `true`. See
    //! [Lazy-loading room members](/client-server-api/#lazy-loading-room-members)
    //! for more information. Defaults to `false`.
    std::optional<bool> includeRedundantMembers{};

    //! A list of room IDs to exclude. If this list is absent then no rooms are excluded. A matching
    //! room will be excluded even if it is listed in the `'rooms'` filter.
    QStringList notRooms{};

    //! A list of room IDs to include. If this list is absent then all rooms are included.
    QStringList rooms{};

    //! If `true`, includes only events with a `url` key in their content. If `false`, excludes
    //! those events. If omitted, `url` key is not considered for filtering.
    std::optional<bool> containsUrl{};
};

template <>
struct JsonObjectConverter<RoomEventFilter> {
    static void dumpTo(QJsonObject& jo, const RoomEventFilter& pod)
    {
        fillJson<EventFilter>(jo, pod);
        addParam<IfNotEmpty>(jo, "unread_thread_notifications"_L1, pod.unreadThreadNotifications);
        addParam<IfNotEmpty>(jo, "lazy_load_members"_L1, pod.lazyLoadMembers);
        addParam<IfNotEmpty>(jo, "include_redundant_members"_L1, pod.includeRedundantMembers);
        addParam<IfNotEmpty>(jo, "not_rooms"_L1, pod.notRooms);
        addParam<IfNotEmpty>(jo, "rooms"_L1, pod.rooms);
        addParam<IfNotEmpty>(jo, "contains_url"_L1, pod.containsUrl);
    }
    static void fillFrom(const QJsonObject& jo, RoomEventFilter& pod)
    {
        fillFromJson<EventFilter>(jo, pod);
        fillFromJson(jo.value("unread_thread_notifications"_L1), pod.unreadThreadNotifications);
        fillFromJson(jo.value("lazy_load_members"_L1), pod.lazyLoadMembers);
        fillFromJson(jo.value("include_redundant_members"_L1), pod.includeRedundantMembers);
        fillFromJson(jo.value("not_rooms"_L1), pod.notRooms);
        fillFromJson(jo.value("rooms"_L1), pod.rooms);
        fillFromJson(jo.value("contains_url"_L1), pod.containsUrl);
    }
};

} // namespace Quotient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include "csapi/definitions/room_event_filter.h"
#include "converters.h"
#include "csapi/definitions/event_filter.h"

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct RoomFilter
    {
        QStringList notRooms;
        QStringList rooms;
        Omittable<RoomEventFilter> ephemeral;
        bool includeLeave;
        Omittable<RoomEventFilter> state;
        Omittable<RoomEventFilter> timeline;
        Omittable<RoomEventFilter> accountData;
    };

    QJsonObject toJson(const RoomFilter& pod);

    template <> struct FromJson<RoomFilter>
    {
        RoomFilter operator()(const QJsonValue& jv);
    };

    struct SyncFilter
    {
        QStringList eventFields;
        QString eventFormat;
        Omittable<Filter> presence;
        Omittable<Filter> accountData;
        Omittable<RoomFilter> room;
    };

    QJsonObject toJson(const SyncFilter& pod);

    template <> struct FromJson<SyncFilter>
    {
        SyncFilter operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

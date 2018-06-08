/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include "csapi/definitions/event_filter.h"
#include "converters.h"

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct RoomEventFilter : Filter
    {
        QStringList notRooms;
        QStringList rooms;
        bool containsUrl;
    };

    QJsonObject toJson(const RoomEventFilter& pod);

    template <> struct FromJson<RoomEventFilter>
    {
        RoomEventFilter operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

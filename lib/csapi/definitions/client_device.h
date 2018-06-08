/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include "converters.h"

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct Device
    {
        QString deviceId;
        QString displayName;
        QString lastSeenIp;
        Omittable<qint64> lastSeenTs;
    };

    QJsonObject toJson(const Device& pod);

    template <> struct FromJson<Device>
    {
        Device operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

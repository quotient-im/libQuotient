/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    /// A client device
    struct Device
    {
        /// Identifier of this device.
        QString deviceId;
        /// Display name set by the user for this device. Absent if no name has been
        /// set.
        QString displayName;
        /// The IP address where this device was last seen. (May be a few minutes out
        /// of date, for efficiency reasons).
        QString lastSeenIp;
        /// The timestamp (in milliseconds since the unix epoch) when this devices
        /// was last seen. (May be a few minutes out of date, for efficiency
        /// reasons).
        Omittable<qint64> lastSeenTs;
    };

    QJsonObject toJson(const Device& pod);

    template <> struct FromJsonObject<Device>
    {
        Device operator()(const QJsonObject& jo) const;
    };

} // namespace QMatrixClient

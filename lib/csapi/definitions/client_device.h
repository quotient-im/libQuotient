/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace QMatrixClient
{

// Data structures

/// A client device
struct Device
{
    /// Identifier of this device.
    QString deviceId;
    /// Display name set by the user for this device. Absent if no name has
    /// beenset.
    QString displayName;
    /// The IP address where this device was last seen. (May be a few minutes
    /// outof date, for efficiency reasons).
    QString lastSeenIp;
    /// The timestamp (in milliseconds since the unix epoch) when this deviceswas
    /// last seen. (May be a few minutes out of date, for efficiencyreasons).
    Omittable<qint64> lastSeenTs;
};

template <>
struct JsonObjectConverter<Device>
{
    static void dumpTo(QJsonObject& jo, const Device& pod);
    static void fillFrom(const QJsonObject& jo, Device& pod);
};

} // namespace QMatrixClient

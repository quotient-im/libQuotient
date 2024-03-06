// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! A client device
struct Device {
    //! Identifier of this device.
    QString deviceId;

    //! Display name set by the user for this device. Absent if no name has been
    //! set.
    QString displayName{};

    //! The IP address where this device was last seen. (May be a few minutes out
    //! of date, for efficiency reasons).
    QString lastSeenIp{};

    //! The timestamp (in milliseconds since the unix epoch) when this devices
    //! was last seen. (May be a few minutes out of date, for efficiency
    //! reasons).
    Omittable<qint64> lastSeenTs{};
};

template <>
struct JsonObjectConverter<Device> {
    static void dumpTo(QJsonObject& jo, const Device& pod)
    {
        addParam<>(jo, QStringLiteral("device_id"), pod.deviceId);
        addParam<IfNotEmpty>(jo, QStringLiteral("display_name"), pod.displayName);
        addParam<IfNotEmpty>(jo, QStringLiteral("last_seen_ip"), pod.lastSeenIp);
        addParam<IfNotEmpty>(jo, QStringLiteral("last_seen_ts"), pod.lastSeenTs);
    }
    static void fillFrom(const QJsonObject& jo, Device& pod)
    {
        fillFromJson(jo.value("device_id"_ls), pod.deviceId);
        fillFromJson(jo.value("display_name"_ls), pod.displayName);
        fillFromJson(jo.value("last_seen_ip"_ls), pod.lastSeenIp);
        fillFromJson(jo.value("last_seen_ts"_ls), pod.lastSeenTs);
    }
};

} // namespace Quotient

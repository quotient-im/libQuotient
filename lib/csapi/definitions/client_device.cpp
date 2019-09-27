/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "client_device.h"

using namespace Quotient;

void JsonObjectConverter<Device>::dumpTo(QJsonObject& jo, const Device& pod)
{
    addParam<>(jo, QStringLiteral("device_id"), pod.deviceId);
    addParam<IfNotEmpty>(jo, QStringLiteral("display_name"), pod.displayName);
    addParam<IfNotEmpty>(jo, QStringLiteral("last_seen_ip"), pod.lastSeenIp);
    addParam<IfNotEmpty>(jo, QStringLiteral("last_seen_ts"), pod.lastSeenTs);
}

void JsonObjectConverter<Device>::fillFrom(const QJsonObject& jo, Device& result)
{
    fromJson(jo.value("device_id"_ls), result.deviceId);
    fromJson(jo.value("display_name"_ls), result.displayName);
    fromJson(jo.value("last_seen_ip"_ls), result.lastSeenIp);
    fromJson(jo.value("last_seen_ts"_ls), result.lastSeenTs);
}

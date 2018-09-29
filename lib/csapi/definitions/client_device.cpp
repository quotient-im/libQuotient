/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "client_device.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const Device& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("device_id"), pod.deviceId);
    addParam<IfNotEmpty>(jo, QStringLiteral("display_name"), pod.displayName);
    addParam<IfNotEmpty>(jo, QStringLiteral("last_seen_ip"), pod.lastSeenIp);
    addParam<IfNotEmpty>(jo, QStringLiteral("last_seen_ts"), pod.lastSeenTs);
    return jo;
}

Device FromJsonObject<Device>::operator()(const QJsonObject& jo) const
{
    Device result;
    result.deviceId =
        fromJson<QString>(jo.value("device_id"_ls));
    result.displayName =
        fromJson<QString>(jo.value("display_name"_ls));
    result.lastSeenIp =
        fromJson<QString>(jo.value("last_seen_ip"_ls));
    result.lastSeenTs =
        fromJson<qint64>(jo.value("last_seen_ts"_ls));

    return result;
}


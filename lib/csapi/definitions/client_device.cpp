/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "client_device.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const Device& pod)
{
    QJsonObject _json;
    addParam<>(_json, QStringLiteral("device_id"), pod.deviceId);
    addParam<IfNotEmpty>(_json, QStringLiteral("display_name"), pod.displayName);
    addParam<IfNotEmpty>(_json, QStringLiteral("last_seen_ip"), pod.lastSeenIp);
    addParam<IfNotEmpty>(_json, QStringLiteral("last_seen_ts"), pod.lastSeenTs);
    return _json;
}

Device FromJson<Device>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    Device result;
    result.deviceId =
        fromJson<QString>(_json.value("device_id"_ls));
    result.displayName =
        fromJson<QString>(_json.value("display_name"_ls));
    result.lastSeenIp =
        fromJson<QString>(_json.value("last_seen_ip"_ls));
    result.lastSeenTs =
        fromJson<qint64>(_json.value("last_seen_ts"_ls));
    
    return result;
}


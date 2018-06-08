/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "client_device.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const Device& pod)
{
    QJsonObject _json;
    addToJson<>(_json, "device_id", pod.deviceId);
    addToJson<IfNotEmpty>(_json, "display_name", pod.displayName);
    addToJson<IfNotEmpty>(_json, "last_seen_ip", pod.lastSeenIp);
    addToJson<IfNotEmpty>(_json, "last_seen_ts", pod.lastSeenTs);
    return _json;
}

Device FromJson<Device>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    Device result;
    result.deviceId =
        fromJson<QString>(_json.value("device_id"));
    result.displayName =
        fromJson<QString>(_json.value("display_name"));
    result.lastSeenIp =
        fromJson<QString>(_json.value("last_seen_ip"));
    result.lastSeenTs =
        fromJson<qint64>(_json.value("last_seen_ts"));
    
    return result;
}


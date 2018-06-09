/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "device_keys.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const DeviceKeys& pod)
{
    QJsonObject _json;
    addParam<>(_json, "user_id", pod.userId);
    addParam<>(_json, "device_id", pod.deviceId);
    addParam<>(_json, "algorithms", pod.algorithms);
    addParam<>(_json, "keys", pod.keys);
    addParam<>(_json, "signatures", pod.signatures);
    return _json;
}

DeviceKeys FromJson<DeviceKeys>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    DeviceKeys result;
    result.userId =
        fromJson<QString>(_json.value("user_id"));
    result.deviceId =
        fromJson<QString>(_json.value("device_id"));
    result.algorithms =
        fromJson<QStringList>(_json.value("algorithms"));
    result.keys =
        fromJson<QHash<QString, QString>>(_json.value("keys"));
    result.signatures =
        fromJson<QHash<QString, QHash<QString, QString>>>(_json.value("signatures"));
    
    return result;
}


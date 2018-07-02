/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "device_keys.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const DeviceKeys& pod)
{
    QJsonObject _json;
    addParam<>(_json, QStringLiteral("user_id"), pod.userId);
    addParam<>(_json, QStringLiteral("device_id"), pod.deviceId);
    addParam<>(_json, QStringLiteral("algorithms"), pod.algorithms);
    addParam<>(_json, QStringLiteral("keys"), pod.keys);
    addParam<>(_json, QStringLiteral("signatures"), pod.signatures);
    return _json;
}

DeviceKeys FromJson<DeviceKeys>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    DeviceKeys result;
    result.userId =
        fromJson<QString>(_json.value("user_id"_ls));
    result.deviceId =
        fromJson<QString>(_json.value("device_id"_ls));
    result.algorithms =
        fromJson<QStringList>(_json.value("algorithms"_ls));
    result.keys =
        fromJson<QHash<QString, QString>>(_json.value("keys"_ls));
    result.signatures =
        fromJson<QHash<QString, QHash<QString, QString>>>(_json.value("signatures"_ls));
    
    return result;
}


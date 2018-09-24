/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "device_keys.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const DeviceKeys& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("user_id"), pod.userId);
    addParam<>(jo, QStringLiteral("device_id"), pod.deviceId);
    addParam<>(jo, QStringLiteral("algorithms"), pod.algorithms);
    addParam<>(jo, QStringLiteral("keys"), pod.keys);
    addParam<>(jo, QStringLiteral("signatures"), pod.signatures);
    return jo;
}

DeviceKeys FromJsonObject<DeviceKeys>::operator()(const QJsonObject& jo) const
{
    DeviceKeys result;
    result.userId =
        fromJson<QString>(jo.value("user_id"_ls));
    result.deviceId =
        fromJson<QString>(jo.value("device_id"_ls));
    result.algorithms =
        fromJson<QStringList>(jo.value("algorithms"_ls));
    result.keys =
        fromJson<QHash<QString, QString>>(jo.value("keys"_ls));
    result.signatures =
        fromJson<QHash<QString, QHash<QString, QString>>>(jo.value("signatures"_ls));

    return result;
}


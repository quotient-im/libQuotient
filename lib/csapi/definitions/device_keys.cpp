/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "device_keys.h"


using namespace QMatrixClient;

    
void JsonObjectConverter<DeviceKeys>::dumpTo(QJsonObject& jo, const DeviceKeys& pod)
{
    addParam<>(jo, QStringLiteral("user_id"), pod.userId);
    addParam<>(jo, QStringLiteral("device_id"), pod.deviceId);
    addParam<>(jo, QStringLiteral("algorithms"), pod.algorithms);
    addParam<>(jo, QStringLiteral("keys"), pod.keys);
    addParam<>(jo, QStringLiteral("signatures"), pod.signatures);

}
    
void JsonObjectConverter<DeviceKeys>::fillFrom(const QJsonObject& jo, DeviceKeys& result)
{
    fromJson(jo.value("user_id"_ls), result.userId);
    fromJson(jo.value("device_id"_ls), result.deviceId);
    fromJson(jo.value("algorithms"_ls), result.algorithms);
    fromJson(jo.value("keys"_ls), result.keys);
    fromJson(jo.value("signatures"_ls), result.signatures);

}
    



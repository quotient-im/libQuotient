/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "user.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const ThirdPartyUser& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("userid"), pod.userid);
    addParam<IfNotEmpty>(_json, QStringLiteral("protocol"), pod.protocol);
    addParam<IfNotEmpty>(_json, QStringLiteral("fields"), pod.fields);
    return _json;
}

ThirdPartyUser FromJson<ThirdPartyUser>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    ThirdPartyUser result;
    result.userid =
        fromJson<QString>(_json.value("userid"_ls));
    result.protocol =
        fromJson<QString>(_json.value("protocol"_ls));
    result.fields =
        fromJson<QJsonObject>(_json.value("fields"_ls));
    
    return result;
}


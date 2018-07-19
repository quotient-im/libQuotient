/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "location.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const ThirdPartyLocation& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("alias"), pod.alias);
    addParam<IfNotEmpty>(_json, QStringLiteral("protocol"), pod.protocol);
    addParam<IfNotEmpty>(_json, QStringLiteral("fields"), pod.fields);
    return _json;
}

ThirdPartyLocation FromJson<ThirdPartyLocation>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    ThirdPartyLocation result;
    result.alias =
        fromJson<QString>(_json.value("alias"_ls));
    result.protocol =
        fromJson<QString>(_json.value("protocol"_ls));
    result.fields =
        fromJson<QJsonObject>(_json.value("fields"_ls));
    
    return result;
}


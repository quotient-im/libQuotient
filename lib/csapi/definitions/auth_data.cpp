/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "auth_data.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const AuthenticationData& pod)
{
    QJsonObject _json = toJson(pod.authInfo);
    addParam<>(_json, QStringLiteral("type"), pod.type);
    addParam<IfNotEmpty>(_json, QStringLiteral("session"), pod.session);
    return _json;
}

AuthenticationData FromJson<AuthenticationData>::operator()(const QJsonValue& jv)
{
    auto _json = jv.toObject();
    AuthenticationData result;
    result.type =
        fromJson<QString>(_json.take("type"_ls));
    result.session =
        fromJson<QString>(_json.take("session"_ls));
    
    result.authInfo = fromJson<QHash<QString, QJsonObject>>(_json);
    return result;
}


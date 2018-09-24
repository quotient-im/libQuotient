/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "auth_data.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const AuthenticationData& pod)
{
    QJsonObject jo = toJson(pod.authInfo);
    addParam<>(jo, QStringLiteral("type"), pod.type);
    addParam<IfNotEmpty>(jo, QStringLiteral("session"), pod.session);
    return jo;
}

AuthenticationData FromJsonObject<AuthenticationData>::operator()(QJsonObject jo) const
{
    AuthenticationData result;
    result.type =
        fromJson<QString>(jo.take("type"_ls));
    result.session =
        fromJson<QString>(jo.take("session"_ls));

    result.authInfo = fromJson<QHash<QString, QJsonObject>>(jo);
    return result;
}


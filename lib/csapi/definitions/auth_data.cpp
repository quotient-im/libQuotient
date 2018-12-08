/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "auth_data.h"

using namespace QMatrixClient;

void JsonObjectConverter<AuthenticationData>::dumpTo(
        QJsonObject& jo, const AuthenticationData& pod)
{
    fillJson(jo, pod.authInfo);
    addParam<>(jo, QStringLiteral("type"), pod.type);
    addParam<IfNotEmpty>(jo, QStringLiteral("session"), pod.session);
}

void JsonObjectConverter<AuthenticationData>::fillFrom(
    QJsonObject jo, AuthenticationData& result)
{
    fromJson(jo.take("type"_ls), result.type);
    fromJson(jo.take("session"_ls), result.session);

    fromJson(jo, result.authInfo);
}


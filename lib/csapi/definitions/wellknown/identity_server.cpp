/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "identity_server.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const IdentityServerInformation& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("base_url"), pod.baseUrl);
    return jo;
}

IdentityServerInformation FromJsonObject<IdentityServerInformation>::operator()(const QJsonObject& jo) const
{
    IdentityServerInformation result;
    result.baseUrl =
        fromJson<QString>(jo.value("base_url"_ls));

    return result;
}


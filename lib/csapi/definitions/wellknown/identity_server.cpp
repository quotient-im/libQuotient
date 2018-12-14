/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "identity_server.h"

using namespace QMatrixClient;

void JsonObjectConverter<IdentityServerInformation>::dumpTo(
        QJsonObject& jo, const IdentityServerInformation& pod)
{
    addParam<>(jo, QStringLiteral("base_url"), pod.baseUrl);
}

void JsonObjectConverter<IdentityServerInformation>::fillFrom(
    const QJsonObject& jo, IdentityServerInformation& result)
{
    fromJson(jo.value("base_url"_ls), result.baseUrl);
}


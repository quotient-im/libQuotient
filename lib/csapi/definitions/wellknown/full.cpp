/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "full.h"


using namespace QMatrixClient;

    
void JsonObjectConverter<DiscoveryInformation>::dumpTo(QJsonObject& jo, const DiscoveryInformation& pod)
{
    fillJson(jo, pod.additionalProperties);
    addParam<>(jo, QStringLiteral("m.homeserver"), pod.homeserver);
    addParam<IfNotEmpty>(jo, QStringLiteral("m.identity_server"), pod.identityServer);

}
    
void JsonObjectConverter<DiscoveryInformation>::fillFrom(QJsonObject jo, DiscoveryInformation& result)
{
    fromJson(jo.take("m.homeserver"_ls), result.homeserver);
    fromJson(jo.take("m.identity_server"_ls), result.identityServer);
    fromJson(jo, result.additionalProperties);

}
    



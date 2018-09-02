/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "homeserver.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const HomeserverInformation& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("base_url"), pod.baseUrl);
    return jo;
}

HomeserverInformation FromJsonObject<HomeserverInformation>::operator()(const QJsonObject& jo) const
{
    HomeserverInformation result;
    result.baseUrl =
        fromJson<QString>(jo.value("base_url"_ls));

    return result;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "homeserver.h"

using namespace QMatrixClient;

void JsonObjectConverter<HomeserverInformation>::dumpTo(
        QJsonObject& jo, const HomeserverInformation& pod)
{
    addParam<>(jo, QStringLiteral("base_url"), pod.baseUrl);
}

void JsonObjectConverter<HomeserverInformation>::fillFrom(
    const QJsonObject& jo, HomeserverInformation& result)
{
    fromJson(jo.value("base_url"_ls), result.baseUrl);
}


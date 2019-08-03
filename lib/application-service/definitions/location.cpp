/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "location.h"

using namespace QMatrixClient;

void JsonObjectConverter<ThirdPartyLocation>::dumpTo(
    QJsonObject& jo, const ThirdPartyLocation& pod)
{
    addParam<>(jo, QStringLiteral("alias"), pod.alias);
    addParam<>(jo, QStringLiteral("protocol"), pod.protocol);
    addParam<>(jo, QStringLiteral("fields"), pod.fields);
}

void JsonObjectConverter<ThirdPartyLocation>::fillFrom(const QJsonObject& jo,
                                                       ThirdPartyLocation& result)
{
    fromJson(jo.value("alias"_ls), result.alias);
    fromJson(jo.value("protocol"_ls), result.protocol);
    fromJson(jo.value("fields"_ls), result.fields);
}

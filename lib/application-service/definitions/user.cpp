/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "user.h"

using namespace QMatrixClient;

void JsonObjectConverter<ThirdPartyUser>::dumpTo(QJsonObject& jo,
                                                 const ThirdPartyUser& pod)
{
    addParam<>(jo, QStringLiteral("userid"), pod.userid);
    addParam<>(jo, QStringLiteral("protocol"), pod.protocol);
    addParam<>(jo, QStringLiteral("fields"), pod.fields);
}

void JsonObjectConverter<ThirdPartyUser>::fillFrom(const QJsonObject& jo,
                                                   ThirdPartyUser& result)
{
    fromJson(jo.value("userid"_ls), result.userid);
    fromJson(jo.value("protocol"_ls), result.protocol);
    fromJson(jo.value("fields"_ls), result.fields);
}

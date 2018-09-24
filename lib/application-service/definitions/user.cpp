/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "user.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const ThirdPartyUser& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("userid"), pod.userid);
    addParam<IfNotEmpty>(jo, QStringLiteral("protocol"), pod.protocol);
    addParam<IfNotEmpty>(jo, QStringLiteral("fields"), pod.fields);
    return jo;
}

ThirdPartyUser FromJsonObject<ThirdPartyUser>::operator()(const QJsonObject& jo) const
{
    ThirdPartyUser result;
    result.userid =
        fromJson<QString>(jo.value("userid"_ls));
    result.protocol =
        fromJson<QString>(jo.value("protocol"_ls));
    result.fields =
        fromJson<QJsonObject>(jo.value("fields"_ls));

    return result;
}


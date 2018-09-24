/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "location.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const ThirdPartyLocation& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("alias"), pod.alias);
    addParam<IfNotEmpty>(jo, QStringLiteral("protocol"), pod.protocol);
    addParam<IfNotEmpty>(jo, QStringLiteral("fields"), pod.fields);
    return jo;
}

ThirdPartyLocation FromJsonObject<ThirdPartyLocation>::operator()(const QJsonObject& jo) const
{
    ThirdPartyLocation result;
    result.alias =
        fromJson<QString>(jo.value("alias"_ls));
    result.protocol =
        fromJson<QString>(jo.value("protocol"_ls));
    result.fields =
        fromJson<QJsonObject>(jo.value("fields"_ls));

    return result;
}


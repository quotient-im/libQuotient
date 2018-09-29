/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "user_identifier.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const UserIdentifier& pod)
{
    QJsonObject jo = toJson(pod.additionalProperties);
    addParam<>(jo, QStringLiteral("type"), pod.type);
    return jo;
}

UserIdentifier FromJsonObject<UserIdentifier>::operator()(QJsonObject jo) const
{
    UserIdentifier result;
    result.type =
        fromJson<QString>(jo.take("type"_ls));

    result.additionalProperties = fromJson<QVariantHash>(jo);
    return result;
}


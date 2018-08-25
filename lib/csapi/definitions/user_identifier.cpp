/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "user_identifier.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const UserIdentifier& pod)
{
    QJsonObject _json = toJson(pod.additionalProperties);
    addParam<>(_json, QStringLiteral("type"), pod.type);
    return _json;
}

UserIdentifier FromJson<UserIdentifier>::operator()(const QJsonValue& jv)
{
    auto _json = jv.toObject();
    UserIdentifier result;
    result.type =
        fromJson<QString>(_json.take("type"_ls));
    
    result.additionalProperties = fromJson<QVariantHash>(_json);
    return result;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_condition.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PushCondition& pod)
{
    QJsonObject _json;
    addParam<>(_json, QStringLiteral("kind"), pod.kind);
    addParam<IfNotEmpty>(_json, QStringLiteral("key"), pod.key);
    addParam<IfNotEmpty>(_json, QStringLiteral("pattern"), pod.pattern);
    addParam<IfNotEmpty>(_json, QStringLiteral("is"), pod.is);
    return _json;
}

PushCondition FromJson<PushCondition>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    PushCondition result;
    result.kind =
        fromJson<QString>(_json.value("kind"_ls));
    result.key =
        fromJson<QString>(_json.value("key"_ls));
    result.pattern =
        fromJson<QString>(_json.value("pattern"_ls));
    result.is =
        fromJson<QString>(_json.value("is"_ls));
    
    return result;
}


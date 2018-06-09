/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_condition.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PushCondition& pod)
{
    QJsonObject _json;
    addParam<>(_json, "kind", pod.kind);
    addParam<IfNotEmpty>(_json, "key", pod.key);
    addParam<IfNotEmpty>(_json, "pattern", pod.pattern);
    addParam<IfNotEmpty>(_json, "is", pod.is);
    return _json;
}

PushCondition FromJson<PushCondition>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    PushCondition result;
    result.kind =
        fromJson<QString>(_json.value("kind"));
    result.key =
        fromJson<QString>(_json.value("key"));
    result.pattern =
        fromJson<QString>(_json.value("pattern"));
    result.is =
        fromJson<QString>(_json.value("is"));
    
    return result;
}


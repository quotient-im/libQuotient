/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_rule.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PushRule& pod)
{
    QJsonObject _json;
    addToJson<>(_json, "actions", pod.actions);
    addToJson<>(_json, "default", pod.isDefault);
    addToJson<>(_json, "enabled", pod.enabled);
    addToJson<>(_json, "rule_id", pod.ruleId);
    addToJson<IfNotEmpty>(_json, "conditions", pod.conditions);
    addToJson<IfNotEmpty>(_json, "pattern", pod.pattern);
    return _json;
}

PushRule FromJson<PushRule>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    PushRule result;
    result.actions =
        fromJson<QVector<QVariant>>(_json.value("actions"));
    result.isDefault =
        fromJson<bool>(_json.value("default"));
    result.enabled =
        fromJson<bool>(_json.value("enabled"));
    result.ruleId =
        fromJson<QString>(_json.value("rule_id"));
    result.conditions =
        fromJson<QVector<PushCondition>>(_json.value("conditions"));
    result.pattern =
        fromJson<QString>(_json.value("pattern"));
    
    return result;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_rule.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const SetTweakAction& pod)
{
    QJsonObject _json = toJson(pod.additionalProperties);
    addParam<>(_json, QStringLiteral("set_tweak"), pod.setTweak);
    return _json;
}

SetTweakAction FromJson<SetTweakAction>::operator()(const QJsonValue& jv)
{
    auto _json = jv.toObject();
    SetTweakAction result;
    result.setTweak =
        fromJson<QString>(_json.take("set_tweak"_ls));
    
    result.additionalProperties = fromJson<QVariantHash>(_json);
    return result;
}

QJsonObject QMatrixClient::toJson(const PushRule& pod)
{
    QJsonObject _json;
    addParam<>(_json, QStringLiteral("actions"), pod.actions);
    addParam<>(_json, QStringLiteral("default"), pod.isDefault);
    addParam<>(_json, QStringLiteral("enabled"), pod.enabled);
    addParam<>(_json, QStringLiteral("rule_id"), pod.ruleId);
    addParam<IfNotEmpty>(_json, QStringLiteral("conditions"), pod.conditions);
    addParam<IfNotEmpty>(_json, QStringLiteral("pattern"), pod.pattern);
    return _json;
}

PushRule FromJson<PushRule>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    PushRule result;
    result.actions =
        fromJson<QVector<QVariant>>(_json.value("actions"_ls));
    result.isDefault =
        fromJson<bool>(_json.value("default"_ls));
    result.enabled =
        fromJson<bool>(_json.value("enabled"_ls));
    result.ruleId =
        fromJson<QString>(_json.value("rule_id"_ls));
    result.conditions =
        fromJson<QVector<PushCondition>>(_json.value("conditions"_ls));
    result.pattern =
        fromJson<QString>(_json.value("pattern"_ls));
    
    return result;
}


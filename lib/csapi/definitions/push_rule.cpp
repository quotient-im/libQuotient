/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_rule.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PushRule& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("actions"), pod.actions);
    addParam<>(jo, QStringLiteral("default"), pod.isDefault);
    addParam<>(jo, QStringLiteral("enabled"), pod.enabled);
    addParam<>(jo, QStringLiteral("rule_id"), pod.ruleId);
    addParam<IfNotEmpty>(jo, QStringLiteral("conditions"), pod.conditions);
    addParam<IfNotEmpty>(jo, QStringLiteral("pattern"), pod.pattern);
    return jo;
}

PushRule FromJsonObject<PushRule>::operator()(const QJsonObject& jo) const
{
    PushRule result;
    result.actions =
        fromJson<QVector<QVariant>>(jo.value("actions"_ls));
    result.isDefault =
        fromJson<bool>(jo.value("default"_ls));
    result.enabled =
        fromJson<bool>(jo.value("enabled"_ls));
    result.ruleId =
        fromJson<QString>(jo.value("rule_id"_ls));
    result.conditions =
        fromJson<QVector<PushCondition>>(jo.value("conditions"_ls));
    result.pattern =
        fromJson<QString>(jo.value("pattern"_ls));

    return result;
}


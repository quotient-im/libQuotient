/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_rule.h"

using namespace QMatrixClient;

void JsonObjectConverter<PushRule>::dumpTo(QJsonObject& jo, const PushRule& pod)
{
    addParam<>(jo, QStringLiteral("actions"), pod.actions);
    addParam<>(jo, QStringLiteral("default"), pod.isDefault);
    addParam<>(jo, QStringLiteral("enabled"), pod.enabled);
    addParam<>(jo, QStringLiteral("rule_id"), pod.ruleId);
    addParam<IfNotEmpty>(jo, QStringLiteral("conditions"), pod.conditions);
    addParam<IfNotEmpty>(jo, QStringLiteral("pattern"), pod.pattern);
}

void JsonObjectConverter<PushRule>::fillFrom(const QJsonObject& jo,
                                             PushRule& result)
{
    fromJson(jo.value("actions"_ls), result.actions);
    fromJson(jo.value("default"_ls), result.isDefault);
    fromJson(jo.value("enabled"_ls), result.enabled);
    fromJson(jo.value("rule_id"_ls), result.ruleId);
    fromJson(jo.value("conditions"_ls), result.conditions);
    fromJson(jo.value("pattern"_ls), result.pattern);
}

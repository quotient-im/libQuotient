/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/csapi/definitions/push_condition.h>

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API PushRule {
    /// The actions to perform when this rule is matched.
    QVector<QVariant> actions;

    /// Whether this is a default rule, or has been set explicitly.
    bool isDefault;

    /// Whether the push rule is enabled or not.
    bool enabled;

    /// The ID of this rule.
    QString ruleId;

    /// The conditions that must hold true for an event in order for a rule to
    /// be applied to an event. A rule with no conditions always matches. Only
    /// applicable to `underride` and `override` rules.
    QVector<PushCondition> conditions{};

    /// The [glob-style pattern](/appendices#glob-style-matching) to match
    /// against. Only applicable to `content` rules.
    QString pattern{};
};

template <>
struct JsonObjectConverter<PushRule> {
    static void dumpTo(QJsonObject& jo, const PushRule& pod)
    {
        addParam<>(jo, QStringLiteral("actions"), pod.actions);
        addParam<>(jo, QStringLiteral("default"), pod.isDefault);
        addParam<>(jo, QStringLiteral("enabled"), pod.enabled);
        addParam<>(jo, QStringLiteral("rule_id"), pod.ruleId);
        addParam<IfNotEmpty>(jo, QStringLiteral("conditions"), pod.conditions);
        addParam<IfNotEmpty>(jo, QStringLiteral("pattern"), pod.pattern);
    }
    static void fillFrom(const QJsonObject& jo, PushRule& pod)
    {
        fillFromJson(jo.value("actions"_ls), pod.actions);
        fillFromJson(jo.value("default"_ls), pod.isDefault);
        fillFromJson(jo.value("enabled"_ls), pod.enabled);
        fillFromJson(jo.value("rule_id"_ls), pod.ruleId);
        fillFromJson(jo.value("conditions"_ls), pod.conditions);
        fillFromJson(jo.value("pattern"_ls), pod.pattern);
    }
};

} // namespace Quotient

// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/push_condition.h>

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API PushRule {
    //! The actions to perform when this rule is matched.
    QVector<QVariant> actions;

    //! Whether this is a default rule, or has been set explicitly.
    bool isDefault;

    //! Whether the push rule is enabled or not.
    bool enabled;

    //! The ID of this rule.
    QString ruleId;

    //! The conditions that must hold true for an event in order for a rule to be
    //! applied to an event. A rule with no conditions always matches. Only
    //! applicable to `underride` and `override` rules.
    QVector<PushCondition> conditions{};

    //! The [glob-style pattern](/appendices#glob-style-matching) to match against.
    //! Only applicable to `content` rules.
    QString pattern{};
};

template <>
struct JsonObjectConverter<PushRule> {
    static void dumpTo(QJsonObject& jo, const PushRule& pod)
    {
        addParam<>(jo, "actions"_L1, pod.actions);
        addParam<>(jo, "default"_L1, pod.isDefault);
        addParam<>(jo, "enabled"_L1, pod.enabled);
        addParam<>(jo, "rule_id"_L1, pod.ruleId);
        addParam<IfNotEmpty>(jo, "conditions"_L1, pod.conditions);
        addParam<IfNotEmpty>(jo, "pattern"_L1, pod.pattern);
    }
    static void fillFrom(const QJsonObject& jo, PushRule& pod)
    {
        fillFromJson(jo.value("actions"_L1), pod.actions);
        fillFromJson(jo.value("default"_L1), pod.isDefault);
        fillFromJson(jo.value("enabled"_L1), pod.enabled);
        fillFromJson(jo.value("rule_id"_L1), pod.ruleId);
        fillFromJson(jo.value("conditions"_L1), pod.conditions);
        fillFromJson(jo.value("pattern"_L1), pod.pattern);
    }
};

} // namespace Quotient

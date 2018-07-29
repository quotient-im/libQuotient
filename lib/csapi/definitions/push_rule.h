/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "csapi/definitions/push_condition.h"
#include <QtCore/QVector>
#include <QtCore/QVariant>
#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    /// Specifies an entry to the 'tweaks' dictionary sent in
    /// the notification request to the Push Gateway.
    /// Tweak parameters, if any, are provided as additional
    /// key-value pairs in this structure.
    struct SetTweakAction
    {
        /// The tweak type. Two predefined types are `sound`
        /// and `highlight`; client applications and Push Gateways
        /// may agree on additional tweak types.
        QString setTweak;
        /// Specifies an entry to the 'tweaks' dictionary sent in
        /// the notification request to the Push Gateway.
        /// Tweak parameters, if any, are provided as additional
        /// key-value pairs in this structure.
        QVariantHash additionalProperties;
    };

    QJsonObject toJson(const SetTweakAction& pod);

    template <> struct FromJson<SetTweakAction>
    {
        SetTweakAction operator()(const QJsonValue& jv);
    };

    struct PushRule
    {
        /// The actions to perform when this rule is matched.
        QVector<QVariant> actions;
        /// Whether this is a default rule, or has been set explicitly.
        bool isDefault;
        /// Whether the push rule is enabled or not.
        bool enabled;
        /// The ID of this rule.
        QString ruleId;
        /// The conditions that must hold true for an event in order for a rule to be
        /// applied to an event. A rule with no conditions always matches. Only
        /// applicable to ``underride`` and ``override`` rules.
        QVector<PushCondition> conditions;
        /// The glob-style pattern to match against.  Only applicable to ``content``
        /// rules.
        QString pattern;
    };

    QJsonObject toJson(const PushRule& pod);

    template <> struct FromJson<PushRule>
    {
        PushRule operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "converters.h"
#include "csapi/definitions/push_condition.h"
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>
#include <QtCore/QVector>

namespace QMatrixClient {
    // Data structures

    struct PushRule {
        /// The actions to perform when this rule is matched.
        QVector<QVariant> actions;
        /// Whether this is a default rule, or has been set explicitly.
        bool isDefault;
        /// Whether the push rule is enabled or not.
        bool enabled;
        /// The ID of this rule.
        QString ruleId;
        /// The conditions that must hold true for an event in order for a rule
        /// to be applied to an event. A rule with no conditions always matches.
        /// Only applicable to ``underride`` and ``override`` rules.
        QVector<PushCondition> conditions;
        /// The glob-style pattern to match against.  Only applicable to
        /// ``content`` rules.
        QString pattern;
    };
    template <> struct JsonObjectConverter<PushRule> {
        static void dumpTo(QJsonObject& jo, const PushRule& pod);
        static void fillFrom(const QJsonObject& jo, PushRule& pod);
    };

} // namespace QMatrixClient

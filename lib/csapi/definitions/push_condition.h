/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {

struct PushCondition {
    /// The kind of condition to apply. See `conditions <#conditions>`_ for
    /// more information on the allowed kinds and how they work.
    QString kind;

    /// Required for ``event_match`` conditions. The dot-separated field of the
    /// event to match.
    ///
    /// Required for ``sender_notification_permission`` conditions. The field in
    /// the power level event the user needs a minimum power level for. Fields
    /// must be specified under the ``notifications`` property in the power
    /// level event's ``content``.
    QString key;

    /// Required for ``event_match`` conditions. The glob-style pattern to
    /// match against. Patterns with no special glob characters should be
    /// treated as having asterisks prepended and appended when testing the
    /// condition.
    QString pattern;

    /// Required for ``room_member_count`` conditions. A decimal integer
    /// optionally prefixed by one of, ==, <, >, >= or <=. A prefix of < matches
    /// rooms where the member count is strictly less than the given number and
    /// so forth. If no prefix is present, this parameter defaults to ==.
    QString is;
};

template <>
struct JsonObjectConverter<PushCondition> {
    static void dumpTo(QJsonObject& jo, const PushCondition& pod)
    {
        addParam<>(jo, QStringLiteral("kind"), pod.kind);
        addParam<IfNotEmpty>(jo, QStringLiteral("key"), pod.key);
        addParam<IfNotEmpty>(jo, QStringLiteral("pattern"), pod.pattern);
        addParam<IfNotEmpty>(jo, QStringLiteral("is"), pod.is);
    }
    static void fillFrom(const QJsonObject& jo, PushCondition& pod)
    {
        fromJson(jo.value("kind"_ls), pod.kind);
        fromJson(jo.value("key"_ls), pod.key);
        fromJson(jo.value("pattern"_ls), pod.pattern);
        fromJson(jo.value("is"_ls), pod.is);
    }
};

} // namespace Quotient

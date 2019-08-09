/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient
{

// Data structures

struct PushCondition
{

    QString kind;
    /// Required for ``event_match`` conditions. The dot-separated field of
    /// theevent to match.
    QString key;
    /// Required for ``event_match`` conditions. The glob-style pattern tomatch
    /// against. Patterns with no special glob characters should betreated as
    /// having asterisks prepended and appended when testing thecondition.
    QString pattern;
    /// Required for ``room_member_count`` conditions. A decimal integeroptionally
    /// prefixed by one of, ==, <, >, >= or <=. A prefix of < matchesrooms where
    /// the member count is strictly less than the given number andso forth. If
    /// no prefix is present, this parameter defaults to ==.
    QString is;
};

template <>
struct JsonObjectConverter<PushCondition>
{
    static void dumpTo(QJsonObject& jo, const PushCondition& pod);
    static void fillFrom(const QJsonObject& jo, PushCondition& pod);
};

} // namespace Quotient

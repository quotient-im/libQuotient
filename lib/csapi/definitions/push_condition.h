/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once



#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct PushCondition
    {
        QString kind;
        /// Required for ``event_match`` conditions. The dot-separated field of the
        /// event to match.
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

    QJsonObject toJson(const PushCondition& pod);

    template <> struct FromJson<PushCondition>
    {
        PushCondition operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

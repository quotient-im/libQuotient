/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct Filter
    {
        /// The maximum number of events to return.
        Omittable<int> limit;
        /// A list of sender IDs to exclude. If this list is absent then no senders are excluded. A matching sender will be excluded even if it is listed in the ``'senders'`` filter.
        QStringList notSenders;
        /// A list of event types to exclude. If this list is absent then no event types are excluded. A matching type will be excluded even if it is listed in the ``'types'`` filter. A '*' can be used as a wildcard to match any sequence of characters.
        QStringList notTypes;
        /// A list of senders IDs to include. If this list is absent then all senders are included.
        QStringList senders;
        /// A list of event types to include. If this list is absent then all event types are included. A ``'*'`` can be used as a wildcard to match any sequence of characters.
        QStringList types;
    };

    QJsonObject toJson(const Filter& pod);

    template <> struct FromJson<Filter>
    {
        Filter operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {

struct EventFilter {
    /// The maximum number of events to return.
    Omittable<int> limit;

    /// A list of sender IDs to exclude. If this list is absent then no senders
    /// are excluded. A matching sender will be excluded even if it is listed in
    /// the `'senders'` filter.
    QStringList notSenders;

    /// A list of event types to exclude. If this list is absent then no event
    /// types are excluded. A matching type will be excluded even if it is
    /// listed in the `'types'` filter. A '*' can be used as a wildcard to match
    /// any sequence of characters.
    QStringList notTypes;

    /// A list of senders IDs to include. If this list is absent then all
    /// senders are included.
    QStringList senders;

    /// A list of event types to include. If this list is absent then all event
    /// types are included. A `'*'` can be used as a wildcard to match any
    /// sequence of characters.
    QStringList types;
};

template <>
struct JsonObjectConverter<EventFilter> {
    static void dumpTo(QJsonObject& jo, const EventFilter& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("limit"), pod.limit);
        addParam<IfNotEmpty>(jo, QStringLiteral("not_senders"), pod.notSenders);
        addParam<IfNotEmpty>(jo, QStringLiteral("not_types"), pod.notTypes);
        addParam<IfNotEmpty>(jo, QStringLiteral("senders"), pod.senders);
        addParam<IfNotEmpty>(jo, QStringLiteral("types"), pod.types);
    }
    static void fillFrom(const QJsonObject& jo, EventFilter& pod)
    {
        fromJson(jo.value("limit"_ls), pod.limit);
        fromJson(jo.value("not_senders"_ls), pod.notSenders);
        fromJson(jo.value("not_types"_ls), pod.notTypes);
        fromJson(jo.value("senders"_ls), pod.senders);
        fromJson(jo.value("types"_ls), pod.types);
    }
};

} // namespace Quotient

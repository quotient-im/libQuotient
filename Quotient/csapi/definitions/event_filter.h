// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API EventFilter {
    //! The maximum number of events to return, must be an integer greater than 0.
    //!
    //! Servers should apply a default value, and impose a maximum value to avoid
    //! resource exhaustion.
    std::optional<int> limit{};

    //! A list of sender IDs to exclude. If this list is absent then no senders are excluded. A
    //! matching sender will be excluded even if it is listed in the `'senders'` filter.
    QStringList notSenders{};

    //! A list of event types to exclude. If this list is absent then no event types are excluded. A
    //! matching type will be excluded even if it is listed in the `'types'` filter. A '*' can be
    //! used as a wildcard to match any sequence of characters.
    QStringList notTypes{};

    //! A list of senders IDs to include. If this list is absent then all senders are included.
    QStringList senders{};

    //! A list of event types to include. If this list is absent then all event types are included.
    //! A `'*'` can be used as a wildcard to match any sequence of characters.
    QStringList types{};
};

template <>
struct JsonObjectConverter<EventFilter> {
    static void dumpTo(QJsonObject& jo, const EventFilter& pod)
    {
        addParam<IfNotEmpty>(jo, "limit"_L1, pod.limit);
        addParam<IfNotEmpty>(jo, "not_senders"_L1, pod.notSenders);
        addParam<IfNotEmpty>(jo, "not_types"_L1, pod.notTypes);
        addParam<IfNotEmpty>(jo, "senders"_L1, pod.senders);
        addParam<IfNotEmpty>(jo, "types"_L1, pod.types);
    }
    static void fillFrom(const QJsonObject& jo, EventFilter& pod)
    {
        fillFromJson(jo.value("limit"_L1), pod.limit);
        fillFromJson(jo.value("not_senders"_L1), pod.notSenders);
        fillFromJson(jo.value("not_types"_L1), pod.notTypes);
        fillFromJson(jo.value("senders"_L1), pod.senders);
        fillFromJson(jo.value("types"_L1), pod.types);
    }
};

} // namespace Quotient

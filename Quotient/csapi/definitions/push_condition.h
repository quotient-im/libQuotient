// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API PushCondition {
    //! The kind of condition to apply. See [conditions](/client-server-api/#conditions-1) for
    //! more information on the allowed kinds and how they work.
    QString kind;

    //! Required for `event_match`, `event_property_is` and `event_property_contains`
    //! conditions. The dot-separated field of the event to match.
    //!
    //! Required for `sender_notification_permission` conditions. The field in
    //! the power level event the user needs a minimum power level for. Fields
    //! must be specified under the `notifications` property in the power level
    //! event's `content`.
    QString key{};

    //! Required for `event_match` conditions. The [glob-style
    //! pattern](/appendices#glob-style-matching) to match against.
    QString pattern{};

    //! Required for `room_member_count` conditions. A decimal integer
    //! optionally prefixed by one of, ==, <, >, >= or <=. A prefix of < matches
    //! rooms where the member count is strictly less than the given number and
    //! so forth. If no prefix is present, this parameter defaults to ==.
    QString is{};

    //! Required for `event_property_is` and `event_property_contains` conditions.
    //! A non-compound [canonical JSON](/appendices#canonical-json) value to match
    //! against.
    QVariant value{};
};

template <>
struct JsonObjectConverter<PushCondition> {
    static void dumpTo(QJsonObject& jo, const PushCondition& pod)
    {
        addParam<>(jo, QStringLiteral("kind"), pod.kind);
        addParam<IfNotEmpty>(jo, QStringLiteral("key"), pod.key);
        addParam<IfNotEmpty>(jo, QStringLiteral("pattern"), pod.pattern);
        addParam<IfNotEmpty>(jo, QStringLiteral("is"), pod.is);
        addParam<IfNotEmpty>(jo, QStringLiteral("value"), pod.value);
    }
    static void fillFrom(const QJsonObject& jo, PushCondition& pod)
    {
        fillFromJson(jo.value("kind"_ls), pod.kind);
        fillFromJson(jo.value("key"_ls), pod.key);
        fillFromJson(jo.value("pattern"_ls), pod.pattern);
        fillFromJson(jo.value("is"_ls), pod.is);
        fillFromJson(jo.value("value"_ls), pod.value);
    }
};

} // namespace Quotient

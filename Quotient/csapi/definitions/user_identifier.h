// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! Identification information for a user
struct QUOTIENT_API UserIdentifier {
    //! The type of identification.  See [Identifier types](/client-server-api/#identifier-types)
    //! for supported values and additional property descriptions.
    QString type;

    QVariantHash additionalProperties{};
};

template <>
struct JsonObjectConverter<UserIdentifier> {
    static void dumpTo(QJsonObject& jo, const UserIdentifier& pod)
    {
        fillJson(jo, pod.additionalProperties);
        addParam<>(jo, "type"_L1, pod.type);
    }
    static void fillFrom(QJsonObject jo, UserIdentifier& pod)
    {
        fillFromJson(jo.take("type"_L1), pod.type);
        fromJson(jo, pod.additionalProperties);
    }
};

} // namespace Quotient

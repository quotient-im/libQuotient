/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {
/// Identification information for a user
struct UserIdentifier {
    /// The type of identification.  See [Identifier
    /// types](/client-server-api/#identifier-types) for supported values and
    /// additional property descriptions.
    QString type;

    /// Identification information for a user
    QVariantHash additionalProperties;
};

template <>
struct JsonObjectConverter<UserIdentifier> {
    static void dumpTo(QJsonObject& jo, const UserIdentifier& pod)
    {
        fillJson(jo, pod.additionalProperties);
        addParam<>(jo, QStringLiteral("type"), pod.type);
    }
    static void fillFrom(QJsonObject jo, UserIdentifier& pod)
    {
        fromJson(jo.take("type"_ls), pod.type);
        fromJson(jo, pod.additionalProperties);
    }
};

} // namespace Quotient

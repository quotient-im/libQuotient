/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API ThirdPartyLocation {
    /// An alias for a matrix room.
    QString alias;

    /// The protocol ID that the third-party location is a part of.
    QString protocol;

    /// Information used to identify this third-party location.
    QJsonObject fields;
};

template <>
struct JsonObjectConverter<ThirdPartyLocation> {
    static void dumpTo(QJsonObject& jo, const ThirdPartyLocation& pod)
    {
        addParam<>(jo, QStringLiteral("alias"), pod.alias);
        addParam<>(jo, QStringLiteral("protocol"), pod.protocol);
        addParam<>(jo, QStringLiteral("fields"), pod.fields);
    }
    static void fillFrom(const QJsonObject& jo, ThirdPartyLocation& pod)
    {
        fillFromJson(jo.value("alias"_ls), pod.alias);
        fillFromJson(jo.value("protocol"_ls), pod.protocol);
        fillFromJson(jo.value("fields"_ls), pod.fields);
    }
};

} // namespace Quotient

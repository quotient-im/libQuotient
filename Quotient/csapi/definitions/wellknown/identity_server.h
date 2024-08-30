// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! Used by clients to discover identity server information.
struct QUOTIENT_API IdentityServerInformation {
    //! The base URL for the identity server for client-server connections.
    QUrl baseUrl;
};

template <>
struct JsonObjectConverter<IdentityServerInformation> {
    static void dumpTo(QJsonObject& jo, const IdentityServerInformation& pod)
    {
        addParam<>(jo, "base_url"_L1, pod.baseUrl);
    }
    static void fillFrom(const QJsonObject& jo, IdentityServerInformation& pod)
    {
        fillFromJson(jo.value("base_url"_L1), pod.baseUrl);
    }
};

} // namespace Quotient

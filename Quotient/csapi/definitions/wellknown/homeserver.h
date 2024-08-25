// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/converters.h>

namespace Quotient {
//! Used by clients to discover homeserver information.
struct QUOTIENT_API HomeserverInformation {
    //! The base URL for the homeserver for client-server connections.
    QUrl baseUrl;
};

template <>
struct JsonObjectConverter<HomeserverInformation> {
    static void dumpTo(QJsonObject& jo, const HomeserverInformation& pod)
    {
        addParam<>(jo, "base_url"_L1, pod.baseUrl);
    }
    static void fillFrom(const QJsonObject& jo, HomeserverInformation& pod)
    {
        fillFromJson(jo.value("base_url"_L1), pod.baseUrl);
    }
};

} // namespace Quotient

// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/wellknown/homeserver.h>
#include <Quotient/csapi/definitions/wellknown/identity_server.h>

#include <Quotient/converters.h>

namespace Quotient {
//! Used by clients to determine the homeserver, identity server, and other
//! optional components they should be interacting with.
struct QUOTIENT_API DiscoveryInformation {
    HomeserverInformation homeserver;

    std::optional<IdentityServerInformation> identityServer{};

    //! Application-dependent keys using Java package naming convention.
    QHash<QString, QJsonObject> additionalProperties{};
};

template <>
struct JsonObjectConverter<DiscoveryInformation> {
    static void dumpTo(QJsonObject& jo, const DiscoveryInformation& pod)
    {
        fillJson(jo, pod.additionalProperties);
        addParam<>(jo, "m.homeserver"_L1, pod.homeserver);
        addParam<IfNotEmpty>(jo, "m.identity_server"_L1, pod.identityServer);
    }
    static void fillFrom(QJsonObject jo, DiscoveryInformation& pod)
    {
        fillFromJson(jo.take("m.homeserver"_L1), pod.homeserver);
        fillFromJson(jo.take("m.identity_server"_L1), pod.identityServer);
        fromJson(jo, pod.additionalProperties);
    }
};

} // namespace Quotient

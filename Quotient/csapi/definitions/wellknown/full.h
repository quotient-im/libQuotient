/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/converters.h>
#include <Quotient/csapi/definitions/wellknown/homeserver.h>
#include <Quotient/csapi/definitions/wellknown/identity_server.h>

namespace Quotient {
/// Used by clients to determine the homeserver, identity server, and other
/// optional components they should be interacting with.
struct DiscoveryInformation {
    /// Used by clients to determine the homeserver, identity server, and other
    /// optional components they should be interacting with.
    HomeserverInformation homeserver;

    /// Used by clients to determine the homeserver, identity server, and other
    /// optional components they should be interacting with.
    Omittable<IdentityServerInformation> identityServer;

    /// Application-dependent keys using Java package naming convention.
    QHash<QString, QJsonObject> additionalProperties;
};

template <>
struct JsonObjectConverter<DiscoveryInformation> {
    static void dumpTo(QJsonObject& jo, const DiscoveryInformation& pod)
    {
        fillJson(jo, pod.additionalProperties);
        addParam<>(jo, QStringLiteral("m.homeserver"), pod.homeserver);
        addParam<IfNotEmpty>(jo, QStringLiteral("m.identity_server"),
                             pod.identityServer);
    }
    static void fillFrom(QJsonObject jo, DiscoveryInformation& pod)
    {
        fromJson(jo.take("m.homeserver"_ls), pod.homeserver);
        fromJson(jo.take("m.identity_server"_ls), pod.identityServer);
        fromJson(jo, pod.additionalProperties);
    }
};

} // namespace Quotient

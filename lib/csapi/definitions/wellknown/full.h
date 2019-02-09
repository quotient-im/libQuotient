/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include <QtCore/QJsonObject>
#include "converters.h"
#include "csapi/definitions/wellknown/homeserver.h"
#include "csapi/definitions/wellknown/identity_server.h"
#include <QtCore/QHash>

namespace QMatrixClient
{
    // Data structures

    /// Used by clients to determine the homeserver, identity server, and other
    /// optional components they should be interacting with.
    struct DiscoveryInformation
    {
        /// Used by clients to determine the homeserver, identity server, and other
        /// optional components they should be interacting with.
        HomeserverInformation homeserver;
        /// Used by clients to determine the homeserver, identity server, and other
        /// optional components they should be interacting with.
        Omittable<IdentityServerInformation> identityServer;
        /// Application-dependent keys using Java package naming convention.
        QHash<QString, QJsonObject> additionalProperties;
    };
    template <> struct JsonObjectConverter<DiscoveryInformation>
    {
        static void dumpTo(QJsonObject& jo, const DiscoveryInformation& pod);
        static void fillFrom(QJsonObject jo, DiscoveryInformation& pod);
    };

} // namespace QMatrixClient

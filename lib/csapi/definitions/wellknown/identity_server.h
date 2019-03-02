/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace QMatrixClient {
    // Data structures

    /// Used by clients to discover identity server information.
    struct IdentityServerInformation {
        /// The base URL for the identity server for client-server connections.
        QString baseUrl;
    };
    template <> struct JsonObjectConverter<IdentityServerInformation> {
        static void dumpTo(QJsonObject& jo,
                           const IdentityServerInformation& pod);
        static void fillFrom(const QJsonObject& jo,
                             IdentityServerInformation& pod);
    };

} // namespace QMatrixClient

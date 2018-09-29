/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"


namespace QMatrixClient
{
    // Data structures

    /// Used by clients to discover identity server information.
    struct IdentityServerInformation
    {
        /// The base URL for the identity server for client-server connections.
        QString baseUrl;
    };

    QJsonObject toJson(const IdentityServerInformation& pod);

    template <> struct FromJsonObject<IdentityServerInformation>
    {
        IdentityServerInformation operator()(const QJsonObject& jo) const;
    };

} // namespace QMatrixClient

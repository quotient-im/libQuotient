/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"


namespace QMatrixClient
{
    // Data structures

    /// Used by clients to discover homeserver information.
    struct HomeserverInformation
    {
        /// The base URL for the homeserver for client-server connections.
        QString baseUrl;
    };

    QJsonObject toJson(const HomeserverInformation& pod);

    template <> struct FromJsonObject<HomeserverInformation>
    {
        HomeserverInformation operator()(const QJsonObject& jo) const;
    };

} // namespace QMatrixClient

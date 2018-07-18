/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once



#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    /// Dictionary of supported third party protocols.
    struct ProtocolMetadata
    {
    };

    QJsonObject toJson(const ProtocolMetadata& pod);

    template <> struct FromJson<ProtocolMetadata>
    {
        ProtocolMetadata operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

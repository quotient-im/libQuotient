/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include "csapi/../application-service/definitions/location.h"
#include "converters.h"
#include <QtCore/QVector>

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    /// List of matched third party locations.
    struct LocationBatch : QVector<Location>
    {
    };

    QJsonObject toJson(const LocationBatch& pod);

    template <> struct FromJson<LocationBatch>
    {
        LocationBatch operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

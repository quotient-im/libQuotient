/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include <QtCore/QJsonObject>

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct Location
    {
        /// An alias for a matrix room.
        QString alias;
        /// The protocol ID that the third party location is a part of.
        QString protocol;
        /// Information used to identify this third party location.
        QJsonObject fields;
    };

    QJsonObject toJson(const Location& pod);

    template <> struct FromJson<Location>
    {
        Location operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

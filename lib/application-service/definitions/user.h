/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include <QtCore/QJsonObject>

namespace QMatrixClient
{
    // Data structures

    struct ThirdPartyUser
    {
        /// A Matrix User ID represting a third party user.
        QString userid;
        /// The protocol ID that the third party location is a part of.
        QString protocol;
        /// Information used to identify this third party location.
        QJsonObject fields;
    };

    QJsonObject toJson(const ThirdPartyUser& pod);

    template <> struct FromJson<ThirdPartyUser>
    {
        ThirdPartyUser operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

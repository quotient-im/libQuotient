/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include <QtCore/QJsonObject>

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct User
    {
        /// A Matrix User ID represting a third party user.
        QString userid;
        /// The protocol ID that the third party location is a part of.
        QString protocol;
        /// Information used to identify this third party location.
        QJsonObject fields;
    };

    QJsonObject toJson(const User& pod);

    template <> struct FromJson<User>
    {
        User operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

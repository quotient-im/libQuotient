/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include "csapi/../application-service/definitions/user.h"
#include "converters.h"
#include <QtCore/QVector>

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    /// List of matched third party users.
    struct UserBatch : QVector<User>
    {
    };

    QJsonObject toJson(const UserBatch& pod);

    template <> struct FromJson<UserBatch>
    {
        UserBatch operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

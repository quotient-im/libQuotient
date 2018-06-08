/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once


#include <QtCore/QHash>

#include "converters.h"

namespace QMatrixClient
{
    // Data structures

    struct DeviceKeys
    {
        QString userId;
        QString deviceId;
        QStringList algorithms;
        QHash<QString, QString> keys;
        QHash<QString, QHash<QString, QString>> signatures;
    };

    QJsonObject toJson(const DeviceKeys& pod);

    template <> struct FromJson<DeviceKeys>
    {
        DeviceKeys operator()(const QJsonValue& jv);
    };

} // namespace QMatrixClient

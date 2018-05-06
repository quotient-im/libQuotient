/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>
#include <QtCore/QHash>

namespace QMatrixClient
{
    // Operations

    class SendToDeviceJob : public BaseJob
    {
        public:
            explicit SendToDeviceJob(const QString& eventType, const QString& txnId, const QHash<QString, QHash<QString, QJsonObject>>& messages = {});
    };
} // namespace QMatrixClient

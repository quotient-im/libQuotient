/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QJsonObject>


namespace QMatrixClient
{
    // Operations

    class SendToDeviceJob : public BaseJob
    {
        public:
            explicit SendToDeviceJob(const QString& eventType, const QString& txnId, const QJsonObject& messages = {});
    };
} // namespace QMatrixClient

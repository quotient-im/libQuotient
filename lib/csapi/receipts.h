/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>


namespace QMatrixClient
{
    // Operations

    class PostReceiptJob : public BaseJob
    {
        public:
            explicit PostReceiptJob(const QString& roomId, const QString& receiptType, const QString& eventId, const QJsonObject& receipt = {});
    };
} // namespace QMatrixClient

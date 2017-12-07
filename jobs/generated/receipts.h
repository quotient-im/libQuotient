/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QJsonObject>
#include <QtCore/QString>


namespace QMatrixClient
{
    // Operations

    class PostReceiptJob : public BaseJob
    {
        public:
            explicit PostReceiptJob(const QString& roomId, const QString& receiptType, const QString& eventId, const QJsonObject& receipt = {});
    };
} // namespace QMatrixClient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QJsonObject>


namespace QMatrixClient
{
    // Operations

    class SendMessageJob : public BaseJob
    {
        public:
            explicit SendMessageJob(const QString& roomId, const QString& eventType, const QString& txnId, const QJsonObject& body = {});
            ~SendMessageJob() override;

            const QString& eventId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

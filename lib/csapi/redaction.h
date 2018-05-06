/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    class RedactEventJob : public BaseJob
    {
        public:
            explicit RedactEventJob(const QString& roomId, const QString& eventId, const QString& txnId, const QString& reason = {});
            ~RedactEventJob() override;

            // Result properties

            const QString& eventId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

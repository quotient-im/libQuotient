/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{
    // Operations

    class RedactEventJob : public BaseJob
    {
        public:
            explicit RedactEventJob(const QString& roomId, const QString& eventId, const QString& txnId, const QString& reason = {});
            ~RedactEventJob() override;

            const QString& eventId() const;
            
        protected:
            Status parseJson(const QJsonDocument& data) override;
            
        private:
            class Private;
            Private* d;
    };
} // namespace QMatrixClient

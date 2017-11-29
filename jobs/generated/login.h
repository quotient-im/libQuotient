/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QString>


namespace QMatrixClient
{

    // Operations

    class LoginJob : public BaseJob
    {
        public:
            explicit LoginJob(QString type, QString user = {}, QString medium = {}, QString address = {}, QString password = {}, QString token = {}, QString deviceId = {}, QString initialDeviceDisplayName = {});

            ~LoginJob() override;

            const QString& userId() const;
            const QString& accessToken() const;
            const QString& homeServer() const;
            const QString& deviceId() const;
            
        protected:
            Status parseJson(const QJsonDocument& data) override;
            
        private:
            class Private;
            Private* d;
    };

} // namespace QMatrixClient

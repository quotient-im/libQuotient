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
            explicit LoginJob(QString type, QString user = {}, QString medium = {}, QString address = {}, QString password = {}, QString token = {}, QString device_id = {}, QString initial_device_display_name = {});

            ~LoginJob() override;

            const QString& user_id() const;
            const QString& access_token() const;
            const QString& home_server() const;
            const QString& device_id() const;
            
        protected:
            Status parseJson(const QJsonDocument& data) override;
            
        private:
            class Private;
            Private* d;
    };

} // namespace QMatrixClient

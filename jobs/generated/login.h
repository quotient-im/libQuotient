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
            explicit LoginJob(const QString& type, const QString& user = {}, const QString& medium = {}, const QString& address = {}, const QString& password = {}, const QString& token = {}, const QString& deviceId = {}, const QString& initialDeviceDisplayName = {});
            ~LoginJob() override;

            const QString& userId() const;
            const QString& accessToken() const;
            const QString& homeServer() const;
            const QString& deviceId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

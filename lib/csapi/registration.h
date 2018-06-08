/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include <QtCore/QJsonObject>

namespace QMatrixClient
{
    // Operations

    class RegisterJob : public BaseJob
    {
        public:
            explicit RegisterJob(const QString& kind = QStringLiteral("user"), const QJsonObject& auth = {}, bool bindEmail = false, const QString& username = {}, const QString& password = {}, const QString& deviceId = {}, const QString& initialDeviceDisplayName = {});
            ~RegisterJob() override;

            // Result properties

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

    class RequestTokenToRegisterJob : public BaseJob
    {
        public:
            explicit RequestTokenToRegisterJob(const QString& clientSecret, const QString& email, int sendAttempt, const QString& idServer = {});
    };

    class ChangePasswordJob : public BaseJob
    {
        public:
            explicit ChangePasswordJob(const QString& newPassword, const QJsonObject& auth = {});
    };

    class RequestTokenToResetPasswordJob : public BaseJob
    {
        public:
            explicit RequestTokenToResetPasswordJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * RequestTokenToResetPasswordJob. This function can be used when
             * a URL for RequestTokenToResetPasswordJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

    };

    class DeactivateAccountJob : public BaseJob
    {
        public:
            explicit DeactivateAccountJob(const QJsonObject& auth = {});
    };

    class CheckUsernameAvailabilityJob : public BaseJob
    {
        public:
            explicit CheckUsernameAvailabilityJob(const QString& username);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * CheckUsernameAvailabilityJob. This function can be used when
             * a URL for CheckUsernameAvailabilityJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& username);

            ~CheckUsernameAvailabilityJob() override;

            // Result properties

            bool available() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

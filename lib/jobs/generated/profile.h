/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"



namespace QMatrixClient
{
    // Operations

    class SetDisplayNameJob : public BaseJob
    {
        public:
            explicit SetDisplayNameJob(const QString& userId, const QString& displayname = {});
    };

    class GetDisplayNameJob : public BaseJob
    {
        public:
            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetDisplayNameJob. This function can be used when
             * a URL for GetDisplayNameJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

            explicit GetDisplayNameJob(const QString& userId);
            ~GetDisplayNameJob() override;

            const QString& displayname() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class SetAvatarUrlJob : public BaseJob
    {
        public:
            explicit SetAvatarUrlJob(const QString& userId, const QString& avatarUrl = {});
    };

    class GetAvatarUrlJob : public BaseJob
    {
        public:
            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetAvatarUrlJob. This function can be used when
             * a URL for GetAvatarUrlJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

            explicit GetAvatarUrlJob(const QString& userId);
            ~GetAvatarUrlJob() override;

            const QString& avatarUrl() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetUserProfileJob : public BaseJob
    {
        public:
            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetUserProfileJob. This function can be used when
             * a URL for GetUserProfileJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

            explicit GetUserProfileJob(const QString& userId);
            ~GetUserProfileJob() override;

            const QString& avatarUrl() const;
            const QString& displayname() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

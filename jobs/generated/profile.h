/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#pragma once

#include "../basejob.h"

#include <QtCore/QString>


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
            explicit GetDisplayNameJob(const QString& userId);
            ~GetDisplayNameJob() override;

            const QString& displayname() const;
            
        protected:
            Status parseJson(const QJsonDocument& data) override;
            
        private:
            class Private;
            Private* d;
    };

    class SetAvatarUrlJob : public BaseJob
    {
        public:
            explicit SetAvatarUrlJob(const QString& userId, const QString& avatarUrl = {});
    };

    class GetAvatarUrlJob : public BaseJob
    {
        public:
            explicit GetAvatarUrlJob(const QString& userId);
            ~GetAvatarUrlJob() override;

            const QString& avatarUrl() const;
            
        protected:
            Status parseJson(const QJsonDocument& data) override;
            
        private:
            class Private;
            Private* d;
    };

    class GetUserProfileJob : public BaseJob
    {
        public:
            explicit GetUserProfileJob(const QString& userId);
            ~GetUserProfileJob() override;

            const QString& avatarUrl() const;
            const QString& displayname() const;
            
        protected:
            Status parseJson(const QJsonDocument& data) override;
            
        private:
            class Private;
            Private* d;
    };
} // namespace QMatrixClient

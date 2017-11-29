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
            explicit SetDisplayNameJob(QString userId, QString displayname = {});

    };
    class GetDisplayNameJob : public BaseJob
    {
        public:
            explicit GetDisplayNameJob(QString userId);

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
            explicit SetAvatarUrlJob(QString userId, QString avatarUrl = {});

    };
    class GetAvatarUrlJob : public BaseJob
    {
        public:
            explicit GetAvatarUrlJob(QString userId);

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
            explicit GetUserProfileJob(QString userId);

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

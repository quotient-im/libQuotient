/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "events/eventloader.h"
#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class SetPresenceJob : public BaseJob
    {
        public:
            explicit SetPresenceJob(const QString& userId, const QString& presence, const QString& statusMsg = {});
    };

    class GetPresenceJob : public BaseJob
    {
        public:
            explicit GetPresenceJob(const QString& userId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetPresenceJob. This function can be used when
             * a URL for GetPresenceJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

            ~GetPresenceJob() override;

            // Result properties

            const QString& presence() const;
            Omittable<int> lastActiveAgo() const;
            const QString& statusMsg() const;
            bool currentlyActive() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class ModifyPresenceListJob : public BaseJob
    {
        public:
            explicit ModifyPresenceListJob(const QString& userId, const QStringList& invite = {}, const QStringList& drop = {});
    };

    class GetPresenceForListJob : public BaseJob
    {
        public:
            explicit GetPresenceForListJob(const QString& userId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetPresenceForListJob. This function can be used when
             * a URL for GetPresenceForListJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId);

            ~GetPresenceForListJob() override;

            // Result properties

            Events&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

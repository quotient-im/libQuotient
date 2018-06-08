/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include "csapi/definitions/sync_filter.h"

namespace QMatrixClient
{
    // Operations

    class DefineFilterJob : public BaseJob
    {
        public:
            explicit DefineFilterJob(const QString& userId, const SyncFilter& filter);
            ~DefineFilterJob() override;

            // Result properties

            const QString& filterId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetFilterJob : public BaseJob
    {
        public:
            explicit GetFilterJob(const QString& userId, const QString& filterId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetFilterJob. This function can be used when
             * a URL for GetFilterJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& filterId);

            ~GetFilterJob() override;

            // Result properties

            const SyncFilter& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

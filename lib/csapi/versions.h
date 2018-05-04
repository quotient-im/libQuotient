/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QStringList>


namespace QMatrixClient
{
    // Operations

    class GetVersionsJob : public BaseJob
    {
        public:
            explicit GetVersionsJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetVersionsJob. This function can be used when
             * a URL for GetVersionsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetVersionsJob() override;

            // Result properties

            const QStringList& versions() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

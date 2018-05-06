/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    class GetTokenOwnerJob : public BaseJob
    {
        public:
            explicit GetTokenOwnerJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetTokenOwnerJob. This function can be used when
             * a URL for GetTokenOwnerJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetTokenOwnerJob() override;

            // Result properties

            const QString& userId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

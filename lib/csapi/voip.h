/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>

namespace QMatrixClient
{
    // Operations

    class GetTurnServerJob : public BaseJob
    {
        public:
            explicit GetTurnServerJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetTurnServerJob. This function can be used when
             * a URL for GetTurnServerJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetTurnServerJob() override;

            // Result properties

            const QJsonObject& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

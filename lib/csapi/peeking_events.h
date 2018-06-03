/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "events/event.h"
#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class PeekEventsJob : public BaseJob
    {
        public:
            explicit PeekEventsJob(const QString& from = {}, Omittable<int> timeout = none, const QString& roomId = {});

            /** Construct a URL out of baseUrl and usual parameters passed to
             * PeekEventsJob. This function can be used when
             * a URL for PeekEventsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& from = {}, Omittable<int> timeout = none, const QString& roomId = {});

            ~PeekEventsJob() override;

            // Result properties

            const QString& begin() const;
            const QString& end() const;
            RoomEvents&& chunk();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

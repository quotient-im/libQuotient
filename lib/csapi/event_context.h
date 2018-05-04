/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "events/event.h"


namespace QMatrixClient
{
    // Operations

    class GetEventContextJob : public BaseJob
    {
        public:
            explicit GetEventContextJob(const QString& roomId, const QString& eventId, int limit = {});

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetEventContextJob. This function can be used when
             * a URL for GetEventContextJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventId, int limit = {});

            ~GetEventContextJob() override;

            // Result properties

            const QString& begin() const;
            const QString& end() const;
            RoomEvents&& eventsBefore();
            RoomEventPtr&& event();
            RoomEvents&& eventsAfter();
            StateEvents&& state();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

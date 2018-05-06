/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "events/event.h"

namespace QMatrixClient
{
    // Operations

    class GetRoomEventsJob : public BaseJob
    {
        public:
            explicit GetRoomEventsJob(const QString& roomId, const QString& from, const QString& dir, const QString& to = {}, int limit = {}, const QString& filter = {});

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetRoomEventsJob. This function can be used when
             * a URL for GetRoomEventsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& from, const QString& dir, const QString& to = {}, int limit = {}, const QString& filter = {});

            ~GetRoomEventsJob() override;

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

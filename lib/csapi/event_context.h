/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include "events/eventloader.h"

namespace QMatrixClient {
    // Operations

    /// Get events and state around the specified event.
    ///
    /// This API returns a number of events that happened just before and
    /// after the specified event. This allows clients to get the context
    /// surrounding an event.
    class GetEventContextJob : public BaseJob
    {
        public:
        /*! Get events and state around the specified event.
         * \param roomId
         *   The room to get events from.
         * \param eventId
         *   The event to get context around.
         * \param limit
         *   The maximum number of events to return. Default: 10.
         */
        explicit GetEventContextJob(const QString& roomId,
                                    const QString& eventId,
                                    Omittable<int> limit = none);

        /*! Construct a URL without creating a full-fledged job object
         *
         * This function can be used when a URL for
         * GetEventContextJob is necessary but the job
         * itself isn't.
         */
        static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                   const QString& eventId,
                                   Omittable<int> limit = none);

        ~GetEventContextJob() override;

        // Result properties

        /// A token that can be used to paginate backwards with.
        const QString& begin() const;
        /// A token that can be used to paginate forwards with.
        const QString& end() const;
        /// A list of room events that happened just before the
        /// requested event, in reverse-chronological order.
        RoomEvents&& eventsBefore();
        /// Details of the requested event.
        RoomEventPtr&& event();
        /// A list of room events that happened just after the
        /// requested event, in chronological order.
        RoomEvents&& eventsAfter();
        /// The state of the room at the last event returned.
        StateEvents&& state();

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

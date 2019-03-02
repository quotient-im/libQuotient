/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include "events/eventloader.h"

namespace QMatrixClient {
    // Operations

    /// Get a list of events for this room
    ///
    /// This API returns a list of message and state events for a room. It uses
    /// pagination query parameters to paginate history in the room.
    class GetRoomEventsJob : public BaseJob
    {
        public:
        /*! Get a list of events for this room
         * \param roomId
         *   The room to get events from.
         * \param from
         *   The token to start returning events from. This token can be
         * obtained from a ``prev_batch`` token returned for each room by the
         * sync API, or from a ``start`` or ``end`` token returned by a previous
         * request to this endpoint. \param dir The direction to return events
         * from. \param to The token to stop returning events at. This token can
         * be obtained from a ``prev_batch`` token returned for each room by the
         * sync endpoint, or from a ``start`` or ``end`` token returned by a
         * previous request to this endpoint. \param limit The maximum number of
         * events to return. Default: 10. \param filter A JSON RoomEventFilter
         * to filter returned events with.
         */
        explicit GetRoomEventsJob(const QString& roomId, const QString& from,
                                  const QString& dir, const QString& to = {},
                                  Omittable<int> limit = none,
                                  const QString& filter = {});

        /*! Construct a URL without creating a full-fledged job object
         *
         * This function can be used when a URL for
         * GetRoomEventsJob is necessary but the job
         * itself isn't.
         */
        static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                   const QString& from, const QString& dir,
                                   const QString& to = {},
                                   Omittable<int> limit = none,
                                   const QString& filter = {});

        ~GetRoomEventsJob() override;

        // Result properties

        /// The token the pagination starts from. If ``dir=b`` this will be
        /// the token supplied in ``from``.
        const QString& begin() const;
        /// The token the pagination ends at. If ``dir=b`` this token should
        /// be used again to request even earlier events.
        const QString& end() const;
        /// A list of room events.
        RoomEvents&& chunk();

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

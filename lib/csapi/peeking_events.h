/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "events/eventloader.h"
#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Listen on the event stream.
 *
 * This will listen for new events related to a particular room and return
 * them to the caller. This will block until an event is received, or until
 * the `timeout` is reached.
 *
 * This API is the same as the normal `/events` endpoint, but can be
 * called by users who have not joined the room.
 *
 * Note that the normal `/events` endpoint has been deprecated. This
 * API will also be deprecated at some point, but its replacement is not
 * yet known.
 */
class PeekEventsJob : public BaseJob {
public:
    /*! \brief Listen on the event stream.
     *
     * \param from
     *   The token to stream from. This token is either from a previous
     *   request to this API or from the initial sync API.
     *
     * \param timeout
     *   The maximum time in milliseconds to wait for an event.
     *
     * \param roomId
     *   The room ID for which events should be returned.
     */
    explicit PeekEventsJob(const QString& from = {},
                           Omittable<int> timeout = none,
                           const QString& roomId = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for PeekEventsJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& from = {},
                               Omittable<int> timeout = none,
                               const QString& roomId = {});

    // Result properties

    /// A token which correlates to the first value in `chunk`. This
    /// is usually the same token supplied to `from=`.
    QString begin() const { return loadFromJson<QString>("start"_ls); }

    /// A token which correlates to the last value in `chunk`. This
    /// token should be used in the next request to `/events`.
    QString end() const { return loadFromJson<QString>("end"_ls); }

    /// An array of events.
    RoomEvents chunk() { return takeFromJson<RoomEvents>("chunk"_ls); }
};

} // namespace Quotient

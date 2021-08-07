/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "events/eventloader.h"
#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Get events and state around the specified event.
 *
 * This API returns a number of events that happened just before and
 * after the specified event. This allows clients to get the context
 * surrounding an event.
 *
 * *Note*: This endpoint supports lazy-loading of room member events. See
 * [Lazy-loading room members](/client-server-api/#lazy-loading-room-members)
 * for more information.
 */
class GetEventContextJob : public BaseJob {
public:
    /*! \brief Get events and state around the specified event.
     *
     * \param roomId
     *   The room to get events from.
     *
     * \param eventId
     *   The event to get context around.
     *
     * \param limit
     *   The maximum number of events to return. Default: 10.
     *
     * \param filter
     *   A JSON `RoomEventFilter` to filter the returned events with. The
     *   filter is only applied to `events_before`, `events_after`, and
     *   `state`. It is not applied to the `event` itself. The filter may
     *   be applied before or/and after the `limit` parameter - whichever the
     *   homeserver prefers.
     *
     *   See [Filtering](/client-server-api/#filtering) for more information.
     */
    explicit GetEventContextJob(const QString& roomId, const QString& eventId,
                                Omittable<int> limit = none,
                                const QString& filter = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetEventContextJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& eventId,
                               Omittable<int> limit = none,
                               const QString& filter = {});

    // Result properties

    /// A token that can be used to paginate backwards with.
    QString begin() const { return loadFromJson<QString>("start"_ls); }

    /// A token that can be used to paginate forwards with.
    QString end() const { return loadFromJson<QString>("end"_ls); }

    /// A list of room events that happened just before the
    /// requested event, in reverse-chronological order.
    RoomEvents eventsBefore()
    {
        return takeFromJson<RoomEvents>("events_before"_ls);
    }

    /// Details of the requested event.
    RoomEventPtr event() { return takeFromJson<RoomEventPtr>("event"_ls); }

    /// A list of room events that happened just after the
    /// requested event, in chronological order.
    RoomEvents eventsAfter()
    {
        return takeFromJson<RoomEvents>("events_after"_ls);
    }

    /// The state of the room at the last event returned.
    StateEvents state() { return takeFromJson<StateEvents>("state"_ls); }
};

} // namespace Quotient

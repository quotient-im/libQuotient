/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "events/eventloader.h"
#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Get a list of events for this room
 *
 * This API returns a list of message and state events for a room. It uses
 * pagination query parameters to paginate history in the room.
 *
 * *Note*: This endpoint supports lazy-loading of room member events. See
 * [Lazy-loading room members](/client-server-api/#lazy-loading-room-members)
 * for more information.
 */
class GetRoomEventsJob : public BaseJob {
public:
    /*! \brief Get a list of events for this room
     *
     * \param roomId
     *   The room to get events from.
     *
     * \param from
     *   The token to start returning events from. This token can be obtained
     *   from a `prev_batch` token returned for each room by the sync API,
     *   or from a `start` or `end` token returned by a previous request
     *   to this endpoint.
     *
     * \param dir
     *   The direction to return events from.
     *
     * \param to
     *   The token to stop returning events at. This token can be obtained from
     *   a `prev_batch` token returned for each room by the sync endpoint,
     *   or from a `start` or `end` token returned by a previous request to
     *   this endpoint.
     *
     * \param limit
     *   The maximum number of events to return. Default: 10.
     *
     * \param filter
     *   A JSON RoomEventFilter to filter returned events with.
     */
    explicit GetRoomEventsJob(const QString& roomId, const QString& from,
                              const QString& dir, const QString& to = {},
                              Omittable<int> limit = none,
                              const QString& filter = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRoomEventsJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& from, const QString& dir,
                               const QString& to = {},
                               Omittable<int> limit = none,
                               const QString& filter = {});

    // Result properties

    /// The token the pagination starts from. If `dir=b` this will be
    /// the token supplied in `from`.
    QString begin() const { return loadFromJson<QString>("start"_ls); }

    /// The token the pagination ends at. If `dir=b` this token should
    /// be used again to request even earlier events.
    QString end() const { return loadFromJson<QString>("end"_ls); }

    /// A list of room events. The order depends on the `dir` parameter.
    /// For `dir=b` events will be in reverse-chronological order,
    /// for `dir=f` in chronological order, so that events start
    /// at the `from` point.
    RoomEvents chunk() { return takeFromJson<RoomEvents>("chunk"_ls); }

    /// A list of state events relevant to showing the `chunk`. For example, if
    /// `lazy_load_members` is enabled in the filter then this may contain
    /// the membership events for the senders of events in the `chunk`.
    ///
    /// Unless `include_redundant_members` is `true`, the server
    /// may remove membership events which would have already been
    /// sent to the client in prior calls to this endpoint, assuming
    /// the membership of those members has not changed.
    StateEvents state() { return takeFromJson<StateEvents>("state"_ls); }
};

} // namespace Quotient

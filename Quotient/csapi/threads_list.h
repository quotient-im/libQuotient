// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/events/roomevent.h>
#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Fetches a list of the threads in a room.
//!
//! This API is used to paginate through the list of the thread roots in a given room.
//!
//! Optionally, the returned list may be filtered according to whether the requesting
//! user has participated in the thread.
class QUOTIENT_API GetThreadRootsJob : public BaseJob {
public:
    //! \param roomId
    //!   The room ID where the thread roots are located.
    //!
    //! \param include
    //!   Optional (default `all`) flag to denote which thread roots are of interest to the caller.
    //!   When `all`, all thread roots found in the room are returned. When `participated`, only
    //!   thread roots for threads the user has [participated
    //!   in](/client-server-api/#server-side-aggregation-of-mthread-relationships) will be
    //!   returned.
    //!
    //! \param limit
    //!   Optional limit for the maximum number of thread roots to include per response. Must be an
    //!   integer greater than zero.
    //!
    //!   Servers should apply a default value, and impose a maximum value to avoid resource
    //!   exhaustion.
    //!
    //! \param from
    //!   A pagination token from a previous result. When not provided, the server starts paginating
    //!   from the most recent event visible to the user (as per history visibility rules;
    //!   topologically).
    explicit GetThreadRootsJob(const QString& roomId, const QString& include = {},
                               Omittable<int> limit = none, const QString& from = {});

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetThreadRootsJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& include = {},
                               Omittable<int> limit = none, const QString& from = {});

    // Result properties

    //! The thread roots, ordered by the `latest_event` in each event's aggregated children. All
    //! events returned include bundled
    //! [aggregations](/client-server-api/#aggregations-of-child-events).
    //!
    //! If the thread root event was sent by an [ignored user](/client-server-api/#ignoring-users),
    //! the event is returned redacted to the caller. This is to simulate the same behaviour of a
    //! client doing aggregation locally on the thread.
    RoomEvents chunk() { return takeFromJson<RoomEvents>("chunk"_ls); }

    //! A token to supply to `from` to keep paginating the responses. Not present when there are
    //! no further results.
    QString nextBatch() const { return loadFromJson<QString>("next_batch"_ls); }
};

} // namespace Quotient

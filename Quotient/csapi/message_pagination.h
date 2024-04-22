// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/events/roomevent.h>
#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Get a list of events for this room
//!
//! This API returns a list of message and state events for a room. It uses
//! pagination query parameters to paginate history in the room.
//!
//! *Note*: This endpoint supports lazy-loading of room member events. See
//! [Lazy-loading room members](/client-server-api/#lazy-loading-room-members) for more information.
class QUOTIENT_API GetRoomEventsJob : public BaseJob {
public:
    //! \param roomId
    //!   The room to get events from.
    //!
    //! \param dir
    //!   The direction to return events from. If this is set to `f`, events
    //!   will be returned in chronological order starting at `from`. If it
    //!   is set to `b`, events will be returned in *reverse* chronological
    //!   order, again starting at `from`.
    //!
    //! \param from
    //!   The token to start returning events from. This token can be obtained
    //!   from a `prev_batch` or `next_batch` token returned by the `/sync` endpoint,
    //!   or from an `end` token returned by a previous request to this endpoint.
    //!
    //!   This endpoint can also accept a value returned as a `start` token
    //!   by a previous request to this endpoint, though servers are not
    //!   required to support this. Clients should not rely on the behaviour.
    //!
    //!   If it is not provided, the homeserver shall return a list of messages
    //!   from the first or last (per the value of the `dir` parameter) visible
    //!   event in the room history for the requesting user.
    //!
    //! \param to
    //!   The token to stop returning events at. This token can be obtained from
    //!   a `prev_batch` or `next_batch` token returned by the `/sync` endpoint,
    //!   or from an `end` token returned by a previous request to this endpoint.
    //!
    //! \param limit
    //!   The maximum number of events to return. Default: 10.
    //!
    //! \param filter
    //!   A JSON RoomEventFilter to filter returned events with.
    explicit GetRoomEventsJob(const QString& roomId, const QString& dir, const QString& from = {},
                              const QString& to = {}, std::optional<int> limit = std::nullopt,
                              const QString& filter = {});

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetRoomEventsJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& dir,
                               const QString& from = {}, const QString& to = {},
                               std::optional<int> limit = std::nullopt, const QString& filter = {});

    // Result properties

    //! A token corresponding to the start of `chunk`. This will be the same as
    //! the value given in `from`.
    QString begin() const { return loadFromJson<QString>("start"_ls); }

    //! A token corresponding to the end of `chunk`. This token can be passed
    //! back to this endpoint to request further events.
    //!
    //! If no further events are available (either because we have
    //! reached the start of the timeline, or because the user does
    //! not have permission to see any more events), this property
    //! is omitted from the response.
    QString end() const { return loadFromJson<QString>("end"_ls); }

    //! A list of room events. The order depends on the `dir` parameter.
    //! For `dir=b` events will be in reverse-chronological order,
    //! for `dir=f` in chronological order. (The exact definition of `chronological`
    //! is dependent on the server implementation.)
    //!
    //! Note that an empty `chunk` does not *necessarily* imply that no more events
    //! are available. Clients should continue to paginate until no `end` property
    //! is returned.
    RoomEvents chunk() { return takeFromJson<RoomEvents>("chunk"_ls); }

    //! A list of state events relevant to showing the `chunk`. For example, if
    //! `lazy_load_members` is enabled in the filter then this may contain
    //! the membership events for the senders of events in the `chunk`.
    //!
    //! Unless `include_redundant_members` is `true`, the server
    //! may remove membership events which would have already been
    //! sent to the client in prior calls to this endpoint, assuming
    //! the membership of those members has not changed.
    RoomEvents state() { return takeFromJson<RoomEvents>("state"_ls); }
};

} // namespace Quotient

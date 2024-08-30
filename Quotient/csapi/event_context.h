// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/events/roomevent.h>
#include <Quotient/events/stateevent.h>
#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Get events and state around the specified event.
//!
//! This API returns a number of events that happened just before and
//! after the specified event. This allows clients to get the context
//! surrounding an event.
//!
//! *Note*: This endpoint supports lazy-loading of room member events. See
//! [Lazy-loading room members](/client-server-api/#lazy-loading-room-members) for more information.
class QUOTIENT_API GetEventContextJob : public BaseJob {
public:
    //! \param roomId
    //!   The room to get events from.
    //!
    //! \param eventId
    //!   The event to get context around.
    //!
    //! \param limit
    //!   The maximum number of context events to return. The limit applies
    //!   to the sum of the `events_before` and `events_after` arrays. The
    //!   requested event ID is always returned in `event` even if `limit` is
    //!   0. Defaults to 10.
    //!
    //! \param filter
    //!   A JSON `RoomEventFilter` to filter the returned events with. The
    //!   filter is only applied to `events_before`, `events_after`, and
    //!   `state`. It is not applied to the `event` itself. The filter may
    //!   be applied before or/and after the `limit` parameter - whichever the
    //!   homeserver prefers.
    //!
    //!   See [Filtering](/client-server-api/#filtering) for more information.
    explicit GetEventContextJob(const QString& roomId, const QString& eventId,
                                std::optional<int> limit = std::nullopt, const QString& filter = {});

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetEventContextJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                               const QString& eventId, std::optional<int> limit = std::nullopt,
                               const QString& filter = {});

    // Result properties

    //! A token that can be used to paginate backwards with.
    QString begin() const { return loadFromJson<QString>("start"_L1); }

    //! A token that can be used to paginate forwards with.
    QString end() const { return loadFromJson<QString>("end"_L1); }

    //! A list of room events that happened just before the
    //! requested event, in reverse-chronological order.
    RoomEvents eventsBefore() { return takeFromJson<RoomEvents>("events_before"_L1); }

    //! Details of the requested event.
    RoomEventPtr event() { return takeFromJson<RoomEventPtr>("event"_L1); }

    //! A list of room events that happened just after the
    //! requested event, in chronological order.
    RoomEvents eventsAfter() { return takeFromJson<RoomEvents>("events_after"_L1); }

    //! The state of the room at the last event returned.
    StateEvents state() { return takeFromJson<StateEvents>("state"_L1); }

    struct Response {
        //! A token that can be used to paginate backwards with.
        QString begin{};

        //! A token that can be used to paginate forwards with.
        QString end{};

        //! A list of room events that happened just before the
        //! requested event, in reverse-chronological order.
        RoomEvents eventsBefore{};

        //! Details of the requested event.
        RoomEventPtr event{};

        //! A list of room events that happened just after the
        //! requested event, in chronological order.
        RoomEvents eventsAfter{};

        //! The state of the room at the last event returned.
        StateEvents state{};
    };
};

template <std::derived_from<GetEventContextJob> JobT>
constexpr inline auto doCollectResponse<JobT> = [](JobT* j) -> GetEventContextJob::Response {
    return { j->begin(), j->end(), j->eventsBefore(), j->event(), j->eventsAfter(), j->state() };
};

} // namespace Quotient

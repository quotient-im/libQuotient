// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Get the closest event ID to the given timestamp
//!
//! Get the ID of the event closest to the given timestamp, in the
//! direction specified by the `dir` parameter.
//!
//! If the server does not have all of the room history and does not have
//! an event suitably close to the requested timestamp, it can use the
//! corresponding [federation
//! endpoint](/server-server-api/#get_matrixfederationv1timestamp_to_eventroomid) to ask other
//! servers for a suitable event.
//!
//! After calling this endpoint, clients can call
//! [`/rooms/{roomId}/context/{eventId}`](#get_matrixclientv3roomsroomidcontexteventid)
//! to obtain a pagination token to retrieve the events around the returned event.
//!
//! The event returned by this endpoint could be an event that the client
//! cannot render, and so may need to paginate in order to locate an event
//! that it can display, which may end up being outside of the client's
//! suitable range.  Clients can employ different strategies to display
//! something reasonable to the user.  For example, the client could try
//! paginating in one direction for a while, while looking at the
//! timestamps of the events that it is paginating through, and if it
//! exceeds a certain difference from the target timestamp, it can try
//! paginating in the opposite direction.  The client could also simply
//! paginate in one direction and inform the user that the closest event
//! found in that direction is outside of the expected range.
class QUOTIENT_API GetEventByTimestampJob : public BaseJob {
public:
    //! \param roomId
    //!   The ID of the room to search
    //!
    //! \param ts
    //!   The timestamp to search from, as given in milliseconds
    //!   since the Unix epoch.
    //!
    //! \param dir
    //!   The direction in which to search.  `f` for forwards, `b` for backwards.
    explicit GetEventByTimestampJob(const QString& roomId, int ts, const QString& dir);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetEventByTimestampJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, int ts, const QString& dir);

    // Result properties

    //! The ID of the event found
    QString eventId() const { return loadFromJson<QString>("event_id"_ls); }

    //! The event's timestamp, in milliseconds since the Unix epoch.
    //! This makes it easy to do a quick comparison to see if the
    //! `event_id` fetched is too far out of range to be useful for your
    //! use case.
    int originServerTimestamp() const { return loadFromJson<int>("origin_server_ts"_ls); }
};

} // namespace Quotient

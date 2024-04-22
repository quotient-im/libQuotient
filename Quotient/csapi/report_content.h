// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Report an event in a joined room as inappropriate.
//!
//! Reports an event as inappropriate to the server, which may then notify
//! the appropriate people. The caller must be joined to the room to report
//! it.
//!
//! It might be possible for clients to deduce whether an event exists by
//! timing the response, as only a report for an event that does exist
//! will require the homeserver to check whether a user is joined to
//! the room. To combat this, homeserver implementations should add
//! a random delay when generating a response.
class QUOTIENT_API ReportContentJob : public BaseJob {
public:
    //! \param roomId
    //!   The room in which the event being reported is located.
    //!
    //! \param eventId
    //!   The event to report.
    //!
    //! \param score
    //!   The score to rate this content as where -100 is most offensive
    //!   and 0 is inoffensive.
    //!
    //! \param reason
    //!   The reason the content is being reported. May be blank.
    explicit ReportContentJob(const QString& roomId, const QString& eventId,
                              std::optional<int> score = std::nullopt, const QString& reason = {});
};

} // namespace Quotient

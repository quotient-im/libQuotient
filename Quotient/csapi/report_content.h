// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Report a room as inappropriate.
//!
//! Reports a room as inappropriate to the server, which may then notify
//! the appropriate people. How such information is delivered is left up to
//! implementations. The caller is not required to be joined to the room to
//! report it.
class QUOTIENT_API ReportRoomJob : public BaseJob
{
public:
    //! \param roomId
    //!   The room being reported.
    //!
    //! \param reason
    //!   The reason the room is being reported. May be blank.
    explicit ReportRoomJob(const QString &roomId, const QString &reason);
};

//! \brief Report an event in a joined room as inappropriate.
//!
//! Reports an event as inappropriate to the server, which may then notify
//! the appropriate people. The caller must be joined to the room to report
//! it.
//!
//! Furthermore, it might be possible for clients to deduce whether a reported
//! event exists by timing the response. This is because only a report for an
//! existing event will require the homeserver to do further processing. To
//! combat this, homeservers MAY add a random delay when generating a response.
class QUOTIENT_API ReportEventJob : public BaseJob
{
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
    //!   The reason the content is being reported.
    explicit ReportEventJob(const QString &roomId, const QString &eventId,
                            std::optional<int> score = std::nullopt, const QString &reason = {});
};

//! \brief Report a user as inappropriate.
//!
//! Reports a user as inappropriate to the server, which may then notify
//! the appropriate people. How such information is delivered is left up to
//! implementations. The caller is not required to be joined to any rooms
//! that the reported user is joined to.
//!
//! Clients may wish to [ignore](#ignoring-users) users after reporting them.
//!
//! Clients could infer whether a reported user exists based on the 404 response.
//! Homeservers that wish to conceal this information MAY return 200 responses
//! regardless of the existence of the reported user.
//!
//! Furthermore, it might be possible for clients to deduce whether a reported
//! user exists by timing the response. This is because only a report for an
//! existing user will require the homeserver to do further processing. To
//! combat this, homeservers MAY add a random delay when generating a response.
class QUOTIENT_API ReportUserJob : public BaseJob
{
public:
    //! \param userId
    //!   The user being reported.
    //!
    //! \param reason
    //!   The reason the room is being reported. May be blank.
    explicit ReportUserJob(const UserId &userId, const QString &reason);
};

} // namespace Quotient

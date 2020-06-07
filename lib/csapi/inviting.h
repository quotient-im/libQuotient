/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Invite a user to participate in a particular room.
 *
 * .. _invite-by-user-id-endpoint:
 *
 * *Note that there are two forms of this API, which are documented separately.
 * This version of the API requires that the inviter knows the Matrix
 * identifier of the invitee. The other is documented in the*
 * `third party invites section`_.
 *
 * This API invites a user to participate in a particular room.
 * They do not start participating in the room until they actually join the
 * room.
 *
 * Only users currently in a particular room can invite other users to
 * join that room.
 *
 * If the user was invited to the room, the homeserver will append a
 * ``m.room.member`` event to the room.
 *
 * .. _third party invites section: `invite-by-third-party-id-endpoint`_
 */
class InviteUserJob : public BaseJob {
public:
    /*! \brief Invite a user to participate in a particular room.
     *
     *
     * \param roomId
     *   The room identifier (not alias) to which to invite the user.
     *
     * \param userId
     *   The fully qualified user ID of the invitee.
     */
    explicit InviteUserJob(const QString& roomId, const QString& userId);
};

} // namespace Quotient

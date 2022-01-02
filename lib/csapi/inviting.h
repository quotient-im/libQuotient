/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Invite a user to participate in a particular room.
 *
 * *Note that there are two forms of this API, which are documented separately.
 * This version of the API requires that the inviter knows the Matrix
 * identifier of the invitee. The other is documented in the*
 * [third party invites
 * section](/client-server-api/#post_matrixclientr0roomsroomidinvite-1).
 *
 * This API invites a user to participate in a particular room.
 * They do not start participating in the room until they actually join the
 * room.
 *
 * Only users currently in a particular room can invite other users to
 * join that room.
 *
 * If the user was invited to the room, the homeserver will append a
 * `m.room.member` event to the room.
 */
class QUOTIENT_API InviteUserJob : public BaseJob {
public:
    /*! \brief Invite a user to participate in a particular room.
     *
     * \param roomId
     *   The room identifier (not alias) to which to invite the user.
     *
     * \param userId
     *   The fully qualified user ID of the invitee.
     *
     * \param reason
     *   Optional reason to be included as the `reason` on the subsequent
     *   membership event.
     */
    explicit InviteUserJob(const QString& roomId, const QString& userId,
                           const QString& reason = {});
};

} // namespace Quotient

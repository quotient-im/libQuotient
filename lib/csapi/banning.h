/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../jobs/basejob.h"

namespace Quotient {

/*! \brief Ban a user in the room.
 *
 * Ban a user in the room. If the user is currently in the room, also kick them.
 *
 * When a user is banned from a room, they may not join it or be invited to it
 * until they are unbanned.
 *
 * The caller must have the required power level in order to perform this
 * operation.
 */
class QUOTIENT_API BanJob : public BaseJob {
public:
    /*! \brief Ban a user in the room.
     *
     * \param roomId
     *   The room identifier (not alias) from which the user should be banned.
     *
     * \param userId
     *   The fully qualified user ID of the user being banned.
     *
     * \param reason
     *   The reason the user has been banned. This will be supplied as the
     * `reason` on the target's updated
     * [`m.room.member`](/client-server-api/#mroommember) event.
     */
    explicit BanJob(const QString& roomId, const QString& userId,
                    const QString& reason = {});
};

/*! \brief Unban a user from the room.
 *
 * Unban a user from the room. This allows them to be invited to the room,
 * and join if they would otherwise be allowed to join according to its join
 * rules.
 *
 * The caller must have the required power level in order to perform this
 * operation.
 */
class QUOTIENT_API UnbanJob : public BaseJob {
public:
    /*! \brief Unban a user from the room.
     *
     * \param roomId
     *   The room identifier (not alias) from which the user should be unbanned.
     *
     * \param userId
     *   The fully qualified user ID of the user being unbanned.
     *
     * \param reason
     *   Optional reason to be included as the `reason` on the subsequent
     *   membership event.
     */
    explicit UnbanJob(const QString& roomId, const QString& userId,
                      const QString& reason = {});
};

} // namespace Quotient

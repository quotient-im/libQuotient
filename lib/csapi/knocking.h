/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Knock on a room, requesting permission to join.
 *
 * *Note that this API takes either a room ID or alias, unlike other membership
 * APIs.*
 *
 * This API "knocks" on the room to ask for permission to join, if the user
 * is allowed to knock on the room. Acceptance of the knock happens out of
 * band from this API, meaning that the client will have to watch for updates
 * regarding the acceptance/rejection of the knock.
 *
 * If the room history settings allow, the user will still be able to see
 * history of the room while being in the "knock" state. The user will have
 * to accept the invitation to join the room (acceptance of knock) to see
 * messages reliably. See the `/join` endpoints for more information about
 * history visibility to the user.
 *
 * The knock will appear as an entry in the response of the
 * [`/sync`](/client-server-api/#get_matrixclientr0sync) API.
 */
class KnockRoomJob : public BaseJob {
public:
    /*! \brief Knock on a room, requesting permission to join.
     *
     * \param roomIdOrAlias
     *   The room identifier or alias to knock upon.
     *
     * \param serverName
     *   The servers to attempt to knock on the room through. One of the servers
     *   must be participating in the room.
     *
     * \param reason
     *   Optional reason to be included as the `reason` on the subsequent
     *   membership event.
     */
    explicit KnockRoomJob(const QString& roomIdOrAlias,
                          const QStringList& serverName = {},
                          const QString& reason = {});

    // Result properties

    /// The knocked room ID.
    QString roomId() const { return loadFromJson<QString>("room_id"_ls); }
};

} // namespace Quotient

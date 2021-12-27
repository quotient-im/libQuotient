/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Strips all non-integrity-critical information out of an event.
 *
 * Strips all information out of an event which isn't critical to the
 * integrity of the server-side representation of the room.
 *
 * This cannot be undone.
 *
 * Any user with a power level greater than or equal to the `m.room.redaction`
 * event power level may send redaction events in the room. If the user's power
 * level greater is also greater than or equal to the `redact` power level
 * of the room, the user may redact events sent by other users.
 *
 * Server administrators may redact events sent by users on their server.
 */
class RedactEventJob : public BaseJob {
public:
    /*! \brief Strips all non-integrity-critical information out of an event.
     *
     * \param roomId
     *   The room from which to redact the event.
     *
     * \param eventId
     *   The ID of the event to redact
     *
     * \param txnId
     *   The transaction ID for this event. Clients should generate a
     *   unique ID; it will be used by the server to ensure idempotency of
     * requests.
     *
     * \param reason
     *   The reason for the event being redacted.
     */
    explicit RedactEventJob(const QString& roomId, const QString& eventId,
                            const QString& txnId, const QString& reason = {});

    // Result properties

    /// A unique identifier for the event.
    QString eventId() const { return loadFromJson<QString>("event_id"_ls); }
};

} // namespace Quotient

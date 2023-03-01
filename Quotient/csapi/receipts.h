/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Send a receipt for the given event ID.
 *
 * This API updates the marker for the given receipt type to the event ID
 * specified.
 */
class QUOTIENT_API PostReceiptJob : public BaseJob {
public:
    /*! \brief Send a receipt for the given event ID.
     *
     * \param roomId
     *   The room in which to send the event.
     *
     * \param receiptType
     *   The type of receipt to send. This can also be `m.fully_read` as an
     *   alternative to
     * [`/read_markers`](/client-server-api/#post_matrixclientv3roomsroomidread_markers).
     *
     *   Note that `m.fully_read` does not appear under `m.receipt`: this
     * endpoint effectively calls `/read_markers` internally when presented with
     * a receipt type of `m.fully_read`.
     *
     * \param eventId
     *   The event ID to acknowledge up to.
     *
     * \param threadId
     *   The root thread event's ID (or `main`) for which
     *   thread this receipt is intended to be under. If
     *   not specified, the read receipt is *unthreaded*
     *   (default).
     */
    explicit PostReceiptJob(const QString& roomId, const QString& receiptType,
                            const QString& eventId,
                            const QString& threadId = {});
};

} // namespace Quotient

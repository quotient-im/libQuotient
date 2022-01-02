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
     *   The type of receipt to send.
     *
     * \param eventId
     *   The event ID to acknowledge up to.
     *
     * \param receipt
     *   Extra receipt information to attach to `content` if any. The
     *   server will automatically set the `ts` field.
     */
    explicit PostReceiptJob(const QString& roomId, const QString& receiptType,
                            const QString& eventId,
                            const QJsonObject& receipt = {});
};

} // namespace Quotient

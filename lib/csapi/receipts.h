/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>

namespace QMatrixClient
{
    // Operations

    /// Send a receipt for the given event ID.
    ///
    /// This API updates the marker for the given receipt type to the event ID
    /// specified.
    class PostReceiptJob : public BaseJob
    {
        public:
            /*! Send a receipt for the given event ID.
             * \param roomId
             *   The room in which to send the event.
             * \param receiptType
             *   The type of receipt to send.
             * \param eventId
             *   The event ID to acknowledge up to.
             * \param receipt
             *   Extra receipt information to attach to ``content`` if any. The
             *   server will automatically set the ``ts`` field.
             */
            explicit PostReceiptJob(const QString& roomId, const QString& receiptType, const QString& eventId, const QJsonObject& receipt = {});
    };
} // namespace QMatrixClient

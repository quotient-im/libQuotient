/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Send a message event to the given room.
 *
 * This endpoint is used to send a message event to a room. Message events
 * allow access to historical events and pagination, making them suited
 * for "once-off" activity in a room.
 *
 * The body of the request should be the content object of the event; the
 * fields in this object will vary depending on the type of event. See
 * [Room Events](/client-server-api/#room-events) for the m. event specification.
 */
class SendMessageJob : public BaseJob {
public:
    /*! \brief Send a message event to the given room.
     *
     * \param roomId
     *   The room to send the event to.
     *
     * \param eventType
     *   The type of event to send.
     *
     * \param txnId
     *   The transaction ID for this event. Clients should generate an
     *   ID unique across requests with the same access token; it will be
     *   used by the server to ensure idempotency of requests.
     *
     * \param body
     *   This endpoint is used to send a message event to a room. Message events
     *   allow access to historical events and pagination, making them suited
     *   for "once-off" activity in a room.
     *
     *   The body of the request should be the content object of the event; the
     *   fields in this object will vary depending on the type of event. See
     *   [Room Events](/client-server-api/#room-events) for the m. event
     * specification.
     */
    explicit SendMessageJob(const QString& roomId, const QString& eventType,
                            const QString& txnId, const QJsonObject& body = {});

    // Result properties

    /// A unique identifier for the event.
    QString eventId() const { return loadFromJson<QString>("event_id"_ls); }
};

} // namespace Quotient

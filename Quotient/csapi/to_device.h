// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Send an event to a given set of devices.
//!
//! This endpoint is used to send send-to-device events to a set of
//! client devices.
class QUOTIENT_API SendToDeviceJob : public BaseJob {
public:
    //! \param eventType
    //!   The type of event to send.
    //!
    //! \param txnId
    //!   The [transaction ID](/client-server-api/#transaction-identifiers) for this event. Clients
    //!   should generate an ID unique across requests with the same access token; it will be used
    //!   by the server to ensure idempotency of requests.
    //!
    //! \param messages
    //!   The messages to send. A map from user ID, to a map from
    //!   device ID to message body. The device ID may also be `*`,
    //!   meaning all known devices for the user.
    explicit SendToDeviceJob(const QString& eventType, const QString& txnId,
                             const QHash<UserId, QHash<QString, QJsonObject>>& messages);
};

} // namespace Quotient

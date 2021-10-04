/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_send.h"

using namespace Quotient;

SendMessageJob::SendMessageJob(const QString& roomId, const QString& eventType,
                               const QString& txnId, const QJsonObject& body)
    : BaseJob(HttpVerb::Put, QStringLiteral("SendMessageJob"),
              makePath("/_matrix/client/r0", "/rooms/", roomId, "/send/",
                       eventType, "/", txnId))
{
    setRequestData(RequestData(toJson(body)));
    addExpectedKey("event_id");
}

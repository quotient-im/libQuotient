/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_send.h"

using namespace Quotient;

SendMessageJob::SendMessageJob(const QString& roomId, const QString& eventType,
                               const QString& txnId, const QJsonObject& content)
    : BaseJob(HttpVerb::Put, QStringLiteral("SendMessageJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/send/",
                       eventType, "/", txnId))
{
    setRequestData({ toJson(content) });
    addExpectedKey("event_id");
}

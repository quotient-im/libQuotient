/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_state.h"

using namespace Quotient;

SetRoomStateWithKeyJob::SetRoomStateWithKeyJob(const QString& roomId,
                                               const QString& eventType,
                                               const QString& stateKey,
                                               const QJsonObject& body)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomStateWithKeyJob"),
              makePath("/_matrix/client/r0", "/rooms/", roomId, "/state/",
                       eventType, "/", stateKey))
{
    setRequestData(RequestData(toJson(body)));
    addExpectedKey("event_id");
}

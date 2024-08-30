// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "room_state.h"

using namespace Quotient;

SetRoomStateWithKeyJob::SetRoomStateWithKeyJob(const QString& roomId, const QString& eventType,
                                               const QString& stateKey, const QJsonObject& content)
    : BaseJob(HttpVerb::Put, u"SetRoomStateWithKeyJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/state/", eventType, "/", stateKey))
{
    setRequestData({ toJson(content) });
    addExpectedKey("event_id");
}

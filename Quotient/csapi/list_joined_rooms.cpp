// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "list_joined_rooms.h"

using namespace Quotient;

QUrl GetJoinedRoomsJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/joined_rooms"));
}

GetJoinedRoomsJob::GetJoinedRoomsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetJoinedRoomsJob"),
              makePath("/_matrix/client/v3", "/joined_rooms"))
{
    addExpectedKey("joined_rooms");
}

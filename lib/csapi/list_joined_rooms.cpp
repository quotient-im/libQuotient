/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "list_joined_rooms.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetJoinedRoomsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/joined_rooms");
}

GetJoinedRoomsJob::GetJoinedRoomsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetJoinedRoomsJob"),
              QStringLiteral("/_matrix/client/r0") % "/joined_rooms")
{
    addExpectedKey("joined_rooms");
}

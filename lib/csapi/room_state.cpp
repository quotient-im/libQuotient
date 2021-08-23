/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_state.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

SetRoomStateWithKeyJob::SetRoomStateWithKeyJob(const QString& roomId,
                                               const QString& eventType,
                                               const QString& stateKey,
                                               const QJsonObject& body)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomStateWithKeyJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/state/" % eventType % "/" % stateKey)
{
    setRequestData(RequestData(toJson(body)));
    addExpectedKey("event_id");
}

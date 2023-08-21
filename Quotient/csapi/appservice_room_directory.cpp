/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "appservice_room_directory.h"

using namespace Quotient;

UpdateAppserviceRoomDirectoryVisibilityJob::UpdateAppserviceRoomDirectoryVisibilityJob(
    const QString& networkId, const QString& roomId, const QString& visibility)
    : BaseJob(HttpVerb::Put,
              QStringLiteral("UpdateAppserviceRoomDirectoryVisibilityJob"),
              makePath("/_matrix/client/v3", "/directory/list/appservice/",
                       networkId, "/", roomId))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("visibility"), visibility);
    setRequestData({ _dataJson });
}

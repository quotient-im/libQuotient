// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "appservice_room_directory.h"

using namespace Quotient;

UpdateAppserviceRoomDirectoryVisibilityJob::UpdateAppserviceRoomDirectoryVisibilityJob(
    const QString& networkId, const QString& roomId, const QString& visibility)
    : BaseJob(HttpVerb::Put, u"UpdateAppserviceRoomDirectoryVisibilityJob"_s,
              makePath("/_matrix/client/v3", "/directory/list/appservice/", networkId, "/", roomId),
              false)
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "visibility"_L1, visibility);
    setRequestData({ _dataJson });
}

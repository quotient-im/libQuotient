/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "appservice_room_directory.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto UpdateAppserviceRoomDirectoryVsibilityJobName = QStringLiteral("UpdateAppserviceRoomDirectoryVsibilityJob");

UpdateAppserviceRoomDirectoryVsibilityJob::UpdateAppserviceRoomDirectoryVsibilityJob(const QString& networkId, const QString& roomId, const QString& visibility)
    : BaseJob(HttpVerb::Put, UpdateAppserviceRoomDirectoryVsibilityJobName,
        basePath % "/directory/list/appservice/" % networkId % "/" % roomId)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("visibility"), visibility);
    setRequestData(_data);
}


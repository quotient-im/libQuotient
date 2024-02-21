// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "room_upgrades.h"

using namespace Quotient;

UpgradeRoomJob::UpgradeRoomJob(const QString& roomId, const QString& newVersion)
    : BaseJob(HttpVerb::Post, QStringLiteral("UpgradeRoomJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/upgrade"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("new_version"), newVersion);
    setRequestData({ _dataJson });
    addExpectedKey("replacement_room");
}

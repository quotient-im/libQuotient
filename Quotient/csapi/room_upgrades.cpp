// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "room_upgrades.h"

using namespace Quotient;

UpgradeRoomJob::UpgradeRoomJob(const QString& roomId, const QString& newVersion)
    : BaseJob(HttpVerb::Post, u"UpgradeRoomJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/upgrade"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "new_version"_L1, newVersion);
    setRequestData({ _dataJson });
    addExpectedKey(u"replacement_room"_s);
}

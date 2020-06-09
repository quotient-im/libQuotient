/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_upgrades.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

UpgradeRoomJob::UpgradeRoomJob(const QString& roomId, const QString& newVersion)
    : BaseJob(HttpVerb::Post, QStringLiteral("UpgradeRoomJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/upgrade")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("new_version"), newVersion);
    setRequestData(std::move(_data));
    addExpectedKey("replacement_room");
}

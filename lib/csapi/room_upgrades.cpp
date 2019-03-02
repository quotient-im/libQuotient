/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "room_upgrades.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class UpgradeRoomJob::Private
{
    public:
    QString replacementRoom;
};

static const auto UpgradeRoomJobName = QStringLiteral("UpgradeRoomJob");

UpgradeRoomJob::UpgradeRoomJob(const QString& roomId, const QString& newVersion)
    : BaseJob(HttpVerb::Post, UpgradeRoomJobName,
              basePath % "/rooms/" % roomId % "/upgrade"),
      d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("new_version"), newVersion);
    setRequestData(_data);
}

UpgradeRoomJob::~UpgradeRoomJob() = default;

const QString& UpgradeRoomJob::replacementRoom() const
{
    return d->replacementRoom;
}

BaseJob::Status UpgradeRoomJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("replacement_room"_ls))
        return { JsonParseError,
                 "The key 'replacement_room' not found in the response" };
    fromJson(json.value("replacement_room"_ls), d->replacementRoom);
    return Success;
}

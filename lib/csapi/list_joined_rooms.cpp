/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "list_joined_rooms.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetJoinedRoomsJob::Private
{
    public:
        QStringList joinedRooms;
};

QUrl GetJoinedRoomsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/joined_rooms");
}

static const auto GetJoinedRoomsJobName = QStringLiteral("GetJoinedRoomsJob");

GetJoinedRoomsJob::GetJoinedRoomsJob()
    : BaseJob(HttpVerb::Get, GetJoinedRoomsJobName,
        basePath % "/joined_rooms")
    , d(new Private)
{
}

GetJoinedRoomsJob::~GetJoinedRoomsJob() = default;

const QStringList& GetJoinedRoomsJob::joinedRooms() const
{
    return d->joinedRooms;
}

BaseJob::Status GetJoinedRoomsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("joined_rooms"_ls))
        return { JsonParseError,
            "The key 'joined_rooms' not found in the response" };
    fromJson(json.value("joined_rooms"_ls), d->joinedRooms);
    return Success;
}


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
        QVector<QString> joinedRooms;
};

QUrl GetJoinedRoomsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/joined_rooms");
}

GetJoinedRoomsJob::GetJoinedRoomsJob()
    : BaseJob(HttpVerb::Get, "GetJoinedRoomsJob",
        basePath % "/joined_rooms")
    , d(new Private)
{
}

GetJoinedRoomsJob::~GetJoinedRoomsJob() = default;

const QVector<QString>& GetJoinedRoomsJob::joinedRooms() const
{
    return d->joinedRooms;
}

BaseJob::Status GetJoinedRoomsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("joined_rooms"))
        return { JsonParseError,
            "The key 'joined_rooms' not found in the response" };
    d->joinedRooms = fromJson<QVector<QString>>(json.value("joined_rooms"));
    return Success;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "list_public_rooms.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetRoomVisibilityOnDirectoryJob::Private
{
    public:
        QString visibility;
};

QUrl GetRoomVisibilityOnDirectoryJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/directory/list/room/" % roomId);
}

GetRoomVisibilityOnDirectoryJob::GetRoomVisibilityOnDirectoryJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, "GetRoomVisibilityOnDirectoryJob",
        basePath % "/directory/list/room/" % roomId, false)
    , d(new Private)
{
}

GetRoomVisibilityOnDirectoryJob::~GetRoomVisibilityOnDirectoryJob() = default;

const QString& GetRoomVisibilityOnDirectoryJob::visibility() const
{
    return d->visibility;
}

BaseJob::Status GetRoomVisibilityOnDirectoryJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->visibility = fromJson<QString>(json.value("visibility"));
    return Success;
}

SetRoomVisibilityOnDirectoryJob::SetRoomVisibilityOnDirectoryJob(const QString& roomId, const QString& visibility)
    : BaseJob(HttpVerb::Put, "SetRoomVisibilityOnDirectoryJob",
        basePath % "/directory/list/room/" % roomId)
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "visibility", visibility);
    setRequestData(_data);
}

namespace QMatrixClient
{
    // Converters

    template <> struct FromJson<GetPublicRoomsJob::PublicRoomsChunk>
    {
        GetPublicRoomsJob::PublicRoomsChunk operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            GetPublicRoomsJob::PublicRoomsChunk result;
            result.aliases =
                fromJson<QStringList>(_json.value("aliases"));
            result.canonicalAlias =
                fromJson<QString>(_json.value("canonical_alias"));
            result.name =
                fromJson<QString>(_json.value("name"));
            result.numJoinedMembers =
                fromJson<qint64>(_json.value("num_joined_members"));
            result.roomId =
                fromJson<QString>(_json.value("room_id"));
            result.topic =
                fromJson<QString>(_json.value("topic"));
            result.worldReadable =
                fromJson<bool>(_json.value("world_readable"));
            result.guestCanJoin =
                fromJson<bool>(_json.value("guest_can_join"));
            result.avatarUrl =
                fromJson<QString>(_json.value("avatar_url"));

            return result;
        }
    };
} // namespace QMatrixClient

class GetPublicRoomsJob::Private
{
    public:
        QVector<PublicRoomsChunk> chunk;
        QString nextBatch;
        QString prevBatch;
        qint64 totalRoomCountEstimate;
};

BaseJob::Query queryToGetPublicRooms(int limit, const QString& since, const QString& server)
{
    BaseJob::Query _q;
    _q.addQueryItem("limit", QString("%1").arg(limit));
    if (!since.isEmpty())
        _q.addQueryItem("since", since);
    if (!server.isEmpty())
        _q.addQueryItem("server", server);
    return _q;
}

QUrl GetPublicRoomsJob::makeRequestUrl(QUrl baseUrl, int limit, const QString& since, const QString& server)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/publicRooms",
            queryToGetPublicRooms(limit, since, server));
}

GetPublicRoomsJob::GetPublicRoomsJob(int limit, const QString& since, const QString& server)
    : BaseJob(HttpVerb::Get, "GetPublicRoomsJob",
        basePath % "/publicRooms",
        queryToGetPublicRooms(limit, since, server),
        {}, false)
    , d(new Private)
{
}

GetPublicRoomsJob::~GetPublicRoomsJob() = default;

const QVector<GetPublicRoomsJob::PublicRoomsChunk>& GetPublicRoomsJob::chunk() const
{
    return d->chunk;
}

const QString& GetPublicRoomsJob::nextBatch() const
{
    return d->nextBatch;
}

const QString& GetPublicRoomsJob::prevBatch() const
{
    return d->prevBatch;
}

qint64 GetPublicRoomsJob::totalRoomCountEstimate() const
{
    return d->totalRoomCountEstimate;
}

BaseJob::Status GetPublicRoomsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("chunk"))
        return { JsonParseError,
            "The key 'chunk' not found in the response" };
    d->chunk = fromJson<QVector<PublicRoomsChunk>>(json.value("chunk"));
    d->nextBatch = fromJson<QString>(json.value("next_batch"));
    d->prevBatch = fromJson<QString>(json.value("prev_batch"));
    d->totalRoomCountEstimate = fromJson<qint64>(json.value("total_room_count_estimate"));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const QueryPublicRoomsJob::Filter& pod)
    {
        QJsonObject _json;
        if (pod.omitted)
            return _json;

        addToJson<IfNotEmpty>(_json, "generic_search_term", pod.genericSearchTerm);
        return _json;
    }

    template <> struct FromJson<QueryPublicRoomsJob::PublicRoomsChunk>
    {
        QueryPublicRoomsJob::PublicRoomsChunk operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            QueryPublicRoomsJob::PublicRoomsChunk result;
            result.aliases =
                fromJson<QStringList>(_json.value("aliases"));
            result.canonicalAlias =
                fromJson<QString>(_json.value("canonical_alias"));
            result.name =
                fromJson<QString>(_json.value("name"));
            result.numJoinedMembers =
                fromJson<qint64>(_json.value("num_joined_members"));
            result.roomId =
                fromJson<QString>(_json.value("room_id"));
            result.topic =
                fromJson<QString>(_json.value("topic"));
            result.worldReadable =
                fromJson<bool>(_json.value("world_readable"));
            result.guestCanJoin =
                fromJson<bool>(_json.value("guest_can_join"));
            result.avatarUrl =
                fromJson<QString>(_json.value("avatar_url"));

            return result;
        }
    };
} // namespace QMatrixClient

class QueryPublicRoomsJob::Private
{
    public:
        QVector<PublicRoomsChunk> chunk;
        QString nextBatch;
        QString prevBatch;
        qint64 totalRoomCountEstimate;
};

BaseJob::Query queryToQueryPublicRooms(const QString& server)
{
    BaseJob::Query _q;
    if (!server.isEmpty())
        _q.addQueryItem("server", server);
    return _q;
}

QueryPublicRoomsJob::QueryPublicRoomsJob(const QString& server, int limit, const QString& since, const Filter& filter)
    : BaseJob(HttpVerb::Post, "QueryPublicRoomsJob",
        basePath % "/publicRooms",
        queryToQueryPublicRooms(server))
    , d(new Private)
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "limit", limit);
    addToJson<IfNotEmpty>(_data, "since", since);
    addToJson<IfNotEmpty>(_data, "filter", filter);
    setRequestData(_data);
}

QueryPublicRoomsJob::~QueryPublicRoomsJob() = default;

const QVector<QueryPublicRoomsJob::PublicRoomsChunk>& QueryPublicRoomsJob::chunk() const
{
    return d->chunk;
}

const QString& QueryPublicRoomsJob::nextBatch() const
{
    return d->nextBatch;
}

const QString& QueryPublicRoomsJob::prevBatch() const
{
    return d->prevBatch;
}

qint64 QueryPublicRoomsJob::totalRoomCountEstimate() const
{
    return d->totalRoomCountEstimate;
}

BaseJob::Status QueryPublicRoomsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("chunk"))
        return { JsonParseError,
            "The key 'chunk' not found in the response" };
    d->chunk = fromJson<QVector<PublicRoomsChunk>>(json.value("chunk"));
    d->nextBatch = fromJson<QString>(json.value("next_batch"));
    d->prevBatch = fromJson<QString>(json.value("prev_batch"));
    d->totalRoomCountEstimate = fromJson<qint64>(json.value("total_room_count_estimate"));
    return Success;
}


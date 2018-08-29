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

static const auto GetRoomVisibilityOnDirectoryJobName = QStringLiteral("GetRoomVisibilityOnDirectoryJob");

GetRoomVisibilityOnDirectoryJob::GetRoomVisibilityOnDirectoryJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, GetRoomVisibilityOnDirectoryJobName,
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
    d->visibility = fromJson<QString>(json.value("visibility"_ls));
    return Success;
}

static const auto SetRoomVisibilityOnDirectoryJobName = QStringLiteral("SetRoomVisibilityOnDirectoryJob");

SetRoomVisibilityOnDirectoryJob::SetRoomVisibilityOnDirectoryJob(const QString& roomId, const QString& visibility)
    : BaseJob(HttpVerb::Put, SetRoomVisibilityOnDirectoryJobName,
        basePath % "/directory/list/room/" % roomId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("visibility"), visibility);
    setRequestData(_data);
}

class GetPublicRoomsJob::Private
{
    public:
        PublicRoomsResponse data;
};

BaseJob::Query queryToGetPublicRooms(Omittable<int> limit, const QString& since, const QString& server)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("since"), since);
    addParam<IfNotEmpty>(_q, QStringLiteral("server"), server);
    return _q;
}

QUrl GetPublicRoomsJob::makeRequestUrl(QUrl baseUrl, Omittable<int> limit, const QString& since, const QString& server)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/publicRooms",
            queryToGetPublicRooms(limit, since, server));
}

static const auto GetPublicRoomsJobName = QStringLiteral("GetPublicRoomsJob");

GetPublicRoomsJob::GetPublicRoomsJob(Omittable<int> limit, const QString& since, const QString& server)
    : BaseJob(HttpVerb::Get, GetPublicRoomsJobName,
        basePath % "/publicRooms",
        queryToGetPublicRooms(limit, since, server),
        {}, false)
    , d(new Private)
{
}

GetPublicRoomsJob::~GetPublicRoomsJob() = default;

const PublicRoomsResponse& GetPublicRoomsJob::data() const
{
    return d->data;
}

BaseJob::Status GetPublicRoomsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("data"_ls))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<PublicRoomsResponse>(json.value("data"_ls));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const QueryPublicRoomsJob::Filter& pod)
    {
        QJsonObject _json;
        addParam<IfNotEmpty>(_json, QStringLiteral("generic_search_term"), pod.genericSearchTerm);
        return _json;
    }

    template <> struct FromJson<QueryPublicRoomsJob::PublicRoomsChunk>
    {
        QueryPublicRoomsJob::PublicRoomsChunk operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            QueryPublicRoomsJob::PublicRoomsChunk result;
            result.aliases =
                fromJson<QStringList>(_json.value("aliases"_ls));
            result.canonicalAlias =
                fromJson<QString>(_json.value("canonical_alias"_ls));
            result.name =
                fromJson<QString>(_json.value("name"_ls));
            result.numJoinedMembers =
                fromJson<qint64>(_json.value("num_joined_members"_ls));
            result.roomId =
                fromJson<QString>(_json.value("room_id"_ls));
            result.topic =
                fromJson<QString>(_json.value("topic"_ls));
            result.worldReadable =
                fromJson<bool>(_json.value("world_readable"_ls));
            result.guestCanJoin =
                fromJson<bool>(_json.value("guest_can_join"_ls));
            result.avatarUrl =
                fromJson<QString>(_json.value("avatar_url"_ls));

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
        Omittable<qint64> totalRoomCountEstimate;
};

BaseJob::Query queryToQueryPublicRooms(const QString& server)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("server"), server);
    return _q;
}

static const auto QueryPublicRoomsJobName = QStringLiteral("QueryPublicRoomsJob");

QueryPublicRoomsJob::QueryPublicRoomsJob(const QString& server, Omittable<int> limit, const QString& since, const Omittable<Filter>& filter)
    : BaseJob(HttpVerb::Post, QueryPublicRoomsJobName,
        basePath % "/publicRooms",
        queryToQueryPublicRooms(server))
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_data, QStringLiteral("since"), since);
    addParam<IfNotEmpty>(_data, QStringLiteral("filter"), filter);
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

Omittable<qint64> QueryPublicRoomsJob::totalRoomCountEstimate() const
{
    return d->totalRoomCountEstimate;
}

BaseJob::Status QueryPublicRoomsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("chunk"_ls))
        return { JsonParseError,
            "The key 'chunk' not found in the response" };
    d->chunk = fromJson<QVector<PublicRoomsChunk>>(json.value("chunk"_ls));
    d->nextBatch = fromJson<QString>(json.value("next_batch"_ls));
    d->prevBatch = fromJson<QString>(json.value("prev_batch"_ls));
    d->totalRoomCountEstimate = fromJson<qint64>(json.value("total_room_count_estimate"_ls));
    return Success;
}


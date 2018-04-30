/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "list_public_rooms.h"

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
    return BaseJob::makeRequestUrl(baseUrl,
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
    if (!visibility.isEmpty())
        _data.insert("visibility", toJson(visibility));
    setRequestData(_data);
}

namespace QMatrixClient
{
    QJsonObject toJson(const GetPublicRoomsJob::PublicRoomsChunk& pod)
    {
        QJsonObject o;
        o.insert("aliases", toJson(pod.aliases));
        o.insert("canonical_alias", toJson(pod.canonicalAlias));
        o.insert("name", toJson(pod.name));
        o.insert("num_joined_members", toJson(pod.numJoinedMembers));
        o.insert("room_id", toJson(pod.roomId));
        o.insert("topic", toJson(pod.topic));
        o.insert("world_readable", toJson(pod.worldReadable));
        o.insert("guest_can_join", toJson(pod.guestCanJoin));
        o.insert("avatar_url", toJson(pod.avatarUrl));
        
        return o;
    }

    template <> struct FromJson<GetPublicRoomsJob::PublicRoomsChunk>
    {
        GetPublicRoomsJob::PublicRoomsChunk operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            GetPublicRoomsJob::PublicRoomsChunk result;
            result.aliases =
                fromJson<QVector<QString>>(o.value("aliases"));
            result.canonicalAlias =
                fromJson<QString>(o.value("canonical_alias"));
            result.name =
                fromJson<QString>(o.value("name"));
            result.numJoinedMembers =
                fromJson<double>(o.value("num_joined_members"));
            result.roomId =
                fromJson<QString>(o.value("room_id"));
            result.topic =
                fromJson<QString>(o.value("topic"));
            result.worldReadable =
                fromJson<bool>(o.value("world_readable"));
            result.guestCanJoin =
                fromJson<bool>(o.value("guest_can_join"));
            result.avatarUrl =
                fromJson<QString>(o.value("avatar_url"));
            
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
        double totalRoomCountEstimate;
};

BaseJob::Query queryToGetPublicRooms(double limit, const QString& since, const QString& server)
{
    BaseJob::Query _q;
    _q.addQueryItem("limit", QString("%1").arg(limit));
    if (!since.isEmpty())
        _q.addQueryItem("since", since);
    if (!server.isEmpty())
        _q.addQueryItem("server", server);
    return _q;
}

QUrl GetPublicRoomsJob::makeRequestUrl(QUrl baseUrl, double limit, const QString& since, const QString& server)
{
    return BaseJob::makeRequestUrl(baseUrl,
            basePath % "/publicRooms",
            queryToGetPublicRooms(limit, since, server));
}

GetPublicRoomsJob::GetPublicRoomsJob(double limit, const QString& since, const QString& server)
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

double GetPublicRoomsJob::totalRoomCountEstimate() const
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
    d->totalRoomCountEstimate = fromJson<double>(json.value("total_room_count_estimate"));
    return Success;
}

namespace QMatrixClient
{
    QJsonObject toJson(const QueryPublicRoomsJob::Filter& pod)
    {
        QJsonObject o;
        o.insert("generic_search_term", toJson(pod.genericSearchTerm));
        
        return o;
    }

    template <> struct FromJson<QueryPublicRoomsJob::Filter>
    {
        QueryPublicRoomsJob::Filter operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            QueryPublicRoomsJob::Filter result;
            result.genericSearchTerm =
                fromJson<QString>(o.value("generic_search_term"));
            
            return result;
        }
    };
} // namespace QMatrixClient

namespace QMatrixClient
{
    QJsonObject toJson(const QueryPublicRoomsJob::PublicRoomsChunk& pod)
    {
        QJsonObject o;
        o.insert("aliases", toJson(pod.aliases));
        o.insert("canonical_alias", toJson(pod.canonicalAlias));
        o.insert("name", toJson(pod.name));
        o.insert("num_joined_members", toJson(pod.numJoinedMembers));
        o.insert("room_id", toJson(pod.roomId));
        o.insert("topic", toJson(pod.topic));
        o.insert("world_readable", toJson(pod.worldReadable));
        o.insert("guest_can_join", toJson(pod.guestCanJoin));
        o.insert("avatar_url", toJson(pod.avatarUrl));
        
        return o;
    }

    template <> struct FromJson<QueryPublicRoomsJob::PublicRoomsChunk>
    {
        QueryPublicRoomsJob::PublicRoomsChunk operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            QueryPublicRoomsJob::PublicRoomsChunk result;
            result.aliases =
                fromJson<QVector<QString>>(o.value("aliases"));
            result.canonicalAlias =
                fromJson<QString>(o.value("canonical_alias"));
            result.name =
                fromJson<QString>(o.value("name"));
            result.numJoinedMembers =
                fromJson<double>(o.value("num_joined_members"));
            result.roomId =
                fromJson<QString>(o.value("room_id"));
            result.topic =
                fromJson<QString>(o.value("topic"));
            result.worldReadable =
                fromJson<bool>(o.value("world_readable"));
            result.guestCanJoin =
                fromJson<bool>(o.value("guest_can_join"));
            result.avatarUrl =
                fromJson<QString>(o.value("avatar_url"));
            
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
        double totalRoomCountEstimate;
};

BaseJob::Query queryToQueryPublicRooms(const QString& server)
{
    BaseJob::Query _q;
    if (!server.isEmpty())
        _q.addQueryItem("server", server);
    return _q;
}

QueryPublicRoomsJob::QueryPublicRoomsJob(const QString& server, double limit, const QString& since, const Filter& filter)
    : BaseJob(HttpVerb::Post, "QueryPublicRoomsJob",
        basePath % "/publicRooms",
        queryToQueryPublicRooms(server))
    , d(new Private)
{
    QJsonObject _data;
    _data.insert("limit", toJson(limit));
    if (!since.isEmpty())
        _data.insert("since", toJson(since));
    _data.insert("filter", toJson(filter));
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

double QueryPublicRoomsJob::totalRoomCountEstimate() const
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
    d->totalRoomCountEstimate = fromJson<double>(json.value("total_room_count_estimate"));
    return Success;
}


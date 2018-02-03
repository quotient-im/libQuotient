/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "list_public_rooms.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

GetPublicRoomsJob::PublicRoomsChunk::operator QJsonObject() const
{
    QJsonObject o;
    o.insert("aliases", toJson(aliases));
    o.insert("canonical_alias", toJson(canonicalAlias));
    o.insert("name", toJson(name));
    o.insert("num_joined_members", toJson(numJoinedMembers));
    o.insert("room_id", toJson(roomId));
    o.insert("topic", toJson(topic));
    o.insert("world_readable", toJson(worldReadable));
    o.insert("guest_can_join", toJson(guestCanJoin));
    o.insert("avatar_url", toJson(avatarUrl));
    
    return o;
}
namespace QMatrixClient
{
    template <> struct FromJson<GetPublicRoomsJob::PublicRoomsChunk>
    {
        GetPublicRoomsJob::PublicRoomsChunk operator()(QJsonValue jv)
        {
            QJsonObject o = jv.toObject();
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

QueryPublicRoomsJob::Filter::operator QJsonObject() const
{
    QJsonObject o;
    o.insert("generic_search_term", toJson(genericSearchTerm));
    
    return o;
}
namespace QMatrixClient
{
    template <> struct FromJson<QueryPublicRoomsJob::Filter>
    {
        QueryPublicRoomsJob::Filter operator()(QJsonValue jv)
        {
            QJsonObject o = jv.toObject();
            QueryPublicRoomsJob::Filter result;
            result.genericSearchTerm =
            fromJson<QString>(o.value("generic_search_term"));
            
            return result;
        }
    };
} // namespace QMatrixClient

QueryPublicRoomsJob::PublicRoomsChunk::operator QJsonObject() const
{
    QJsonObject o;
    o.insert("aliases", toJson(aliases));
    o.insert("canonical_alias", toJson(canonicalAlias));
    o.insert("name", toJson(name));
    o.insert("num_joined_members", toJson(numJoinedMembers));
    o.insert("room_id", toJson(roomId));
    o.insert("topic", toJson(topic));
    o.insert("world_readable", toJson(worldReadable));
    o.insert("guest_can_join", toJson(guestCanJoin));
    o.insert("avatar_url", toJson(avatarUrl));
    
    return o;
}
namespace QMatrixClient
{
    template <> struct FromJson<QueryPublicRoomsJob::PublicRoomsChunk>
    {
        QueryPublicRoomsJob::PublicRoomsChunk operator()(QJsonValue jv)
        {
            QJsonObject o = jv.toObject();
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


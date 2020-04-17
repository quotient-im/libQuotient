/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "list_public_rooms.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetRoomVisibilityOnDirectoryJob::Private {
public:
    QString visibility;
};

QUrl GetRoomVisibilityOnDirectoryJob::makeRequestUrl(QUrl baseUrl,
                                                     const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/directory/list/room/" % roomId);
}

GetRoomVisibilityOnDirectoryJob::GetRoomVisibilityOnDirectoryJob(
    const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomVisibilityOnDirectoryJob"),
              basePath % "/directory/list/room/" % roomId, false)
    , d(new Private)
{}

GetRoomVisibilityOnDirectoryJob::~GetRoomVisibilityOnDirectoryJob() = default;

const QString& GetRoomVisibilityOnDirectoryJob::visibility() const
{
    return d->visibility;
}

BaseJob::Status
GetRoomVisibilityOnDirectoryJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("visibility"_ls), d->visibility);

    return Success;
}

SetRoomVisibilityOnDirectoryJob::SetRoomVisibilityOnDirectoryJob(
    const QString& roomId, const QString& visibility)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomVisibilityOnDirectoryJob"),
              basePath % "/directory/list/room/" % roomId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("visibility"), visibility);
    setRequestData(_data);
}

class GetPublicRoomsJob::Private {
public:
    PublicRoomsResponse data;
};

BaseJob::Query queryToGetPublicRooms(Omittable<int> limit, const QString& since,
                                     const QString& server)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("since"), since);
    addParam<IfNotEmpty>(_q, QStringLiteral("server"), server);
    return _q;
}

QUrl GetPublicRoomsJob::makeRequestUrl(QUrl baseUrl, Omittable<int> limit,
                                       const QString& since,
                                       const QString& server)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/publicRooms",
                                   queryToGetPublicRooms(limit, since, server));
}

GetPublicRoomsJob::GetPublicRoomsJob(Omittable<int> limit, const QString& since,
                                     const QString& server)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPublicRoomsJob"),
              basePath % "/publicRooms",
              queryToGetPublicRooms(limit, since, server), {}, false)
    , d(new Private)
{}

GetPublicRoomsJob::~GetPublicRoomsJob() = default;

const PublicRoomsResponse& GetPublicRoomsJob::data() const { return d->data; }

BaseJob::Status GetPublicRoomsJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

// Converters
namespace Quotient {

template <>
struct JsonObjectConverter<QueryPublicRoomsJob::Filter> {
    static void dumpTo(QJsonObject& jo, const QueryPublicRoomsJob::Filter& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("generic_search_term"),
                             pod.genericSearchTerm);
    }
};

} // namespace Quotient

class QueryPublicRoomsJob::Private {
public:
    PublicRoomsResponse data;
};

BaseJob::Query queryToQueryPublicRooms(const QString& server)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("server"), server);
    return _q;
}

QueryPublicRoomsJob::QueryPublicRoomsJob(const QString& server,
                                         Omittable<int> limit,
                                         const QString& since,
                                         const Omittable<Filter>& filter,
                                         Omittable<bool> includeAllNetworks,
                                         const QString& thirdPartyInstanceId)
    : BaseJob(HttpVerb::Post, QStringLiteral("QueryPublicRoomsJob"),
              basePath % "/publicRooms", queryToQueryPublicRooms(server))
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_data, QStringLiteral("since"), since);
    addParam<IfNotEmpty>(_data, QStringLiteral("filter"), filter);
    addParam<IfNotEmpty>(_data, QStringLiteral("include_all_networks"),
                         includeAllNetworks);
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_instance_id"),
                         thirdPartyInstanceId);
    setRequestData(_data);
}

QueryPublicRoomsJob::~QueryPublicRoomsJob() = default;

const PublicRoomsResponse& QueryPublicRoomsJob::data() const { return d->data; }

BaseJob::Status QueryPublicRoomsJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

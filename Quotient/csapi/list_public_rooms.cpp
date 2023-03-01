/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "list_public_rooms.h"

using namespace Quotient;

QUrl GetRoomVisibilityOnDirectoryJob::makeRequestUrl(QUrl baseUrl,
                                                     const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3",
                                            "/directory/list/room/", roomId));
}

GetRoomVisibilityOnDirectoryJob::GetRoomVisibilityOnDirectoryJob(
    const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomVisibilityOnDirectoryJob"),
              makePath("/_matrix/client/v3", "/directory/list/room/", roomId),
              false)
{}

SetRoomVisibilityOnDirectoryJob::SetRoomVisibilityOnDirectoryJob(
    const QString& roomId, const QString& visibility)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomVisibilityOnDirectoryJob"),
              makePath("/_matrix/client/v3", "/directory/list/room/", roomId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("visibility"), visibility);
    setRequestData({ _dataJson });
}

auto queryToGetPublicRooms(Omittable<int> limit, const QString& since,
                           const QString& server)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("since"), since);
    addParam<IfNotEmpty>(_q, QStringLiteral("server"), server);
    return _q;
}

QUrl GetPublicRoomsJob::makeRequestUrl(QUrl baseUrl, Omittable<int> limit,
                                       const QString& since,
                                       const QString& server)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3",
                                            "/publicRooms"),
                                   queryToGetPublicRooms(limit, since, server));
}

GetPublicRoomsJob::GetPublicRoomsJob(Omittable<int> limit, const QString& since,
                                     const QString& server)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPublicRoomsJob"),
              makePath("/_matrix/client/v3", "/publicRooms"),
              queryToGetPublicRooms(limit, since, server), {}, false)
{
    addExpectedKey("chunk");
}

auto queryToQueryPublicRooms(const QString& server)
{
    QUrlQuery _q;
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
              makePath("/_matrix/client/v3", "/publicRooms"),
              queryToQueryPublicRooms(server))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("since"), since);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("filter"), filter);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("include_all_networks"),
                         includeAllNetworks);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("third_party_instance_id"),
                         thirdPartyInstanceId);
    setRequestData({ _dataJson });
    addExpectedKey("chunk");
}

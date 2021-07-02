/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "list_public_rooms.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetRoomVisibilityOnDirectoryJob::makeRequestUrl(QUrl baseUrl,
                                                     const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/directory/list/room/" % roomId);
}

GetRoomVisibilityOnDirectoryJob::GetRoomVisibilityOnDirectoryJob(
    const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomVisibilityOnDirectoryJob"),
              QStringLiteral("/_matrix/client/r0") % "/directory/list/room/"
                  % roomId,
              false)
{}

SetRoomVisibilityOnDirectoryJob::SetRoomVisibilityOnDirectoryJob(
    const QString& roomId, const QString& visibility)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomVisibilityOnDirectoryJob"),
              QStringLiteral("/_matrix/client/r0") % "/directory/list/room/"
                  % roomId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("visibility"), visibility);
    setRequestData(std::move(_data));
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
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/publicRooms",
                                   queryToGetPublicRooms(limit, since, server));
}

GetPublicRoomsJob::GetPublicRoomsJob(Omittable<int> limit, const QString& since,
                                     const QString& server)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPublicRoomsJob"),
              QStringLiteral("/_matrix/client/r0") % "/publicRooms",
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
              QStringLiteral("/_matrix/client/r0") % "/publicRooms",
              queryToQueryPublicRooms(server))
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_data, QStringLiteral("since"), since);
    addParam<IfNotEmpty>(_data, QStringLiteral("filter"), filter);
    addParam<IfNotEmpty>(_data, QStringLiteral("include_all_networks"),
                         includeAllNetworks);
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_instance_id"),
                         thirdPartyInstanceId);
    setRequestData(std::move(_data));
    addExpectedKey("chunk");
}

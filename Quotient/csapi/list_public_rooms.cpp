// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "list_public_rooms.h"

using namespace Quotient;

QUrl GetRoomVisibilityOnDirectoryJob::makeRequestUrl(const HomeserverData& hsData,
                                                     const QString& roomId)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/directory/list/room/", roomId));
}

GetRoomVisibilityOnDirectoryJob::GetRoomVisibilityOnDirectoryJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, u"GetRoomVisibilityOnDirectoryJob"_s,
              makePath("/_matrix/client/v3", "/directory/list/room/", roomId), false)
{}

SetRoomVisibilityOnDirectoryJob::SetRoomVisibilityOnDirectoryJob(const QString& roomId,
                                                                 const QString& visibility)
    : BaseJob(HttpVerb::Put, u"SetRoomVisibilityOnDirectoryJob"_s,
              makePath("/_matrix/client/v3", "/directory/list/room/", roomId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "visibility"_L1, visibility);
    setRequestData({ _dataJson });
}

auto queryToGetPublicRooms(std::optional<int> limit, const QString& since, const QString& server)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"limit"_s, limit);
    addParam<IfNotEmpty>(_q, u"since"_s, since);
    addParam<IfNotEmpty>(_q, u"server"_s, server);
    return _q;
}

QUrl GetPublicRoomsJob::makeRequestUrl(const HomeserverData& hsData, std::optional<int> limit,
                                       const QString& since, const QString& server)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/publicRooms"),
                                   queryToGetPublicRooms(limit, since, server));
}

GetPublicRoomsJob::GetPublicRoomsJob(std::optional<int> limit, const QString& since,
                                     const QString& server)
    : BaseJob(HttpVerb::Get, u"GetPublicRoomsJob"_s, makePath("/_matrix/client/v3", "/publicRooms"),
              queryToGetPublicRooms(limit, since, server), {}, false)
{
    addExpectedKey("chunk");
}

auto queryToQueryPublicRooms(const QString& server)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"server"_s, server);
    return _q;
}

QueryPublicRoomsJob::QueryPublicRoomsJob(const QString& server, std::optional<int> limit,
                                         const QString& since, const std::optional<Filter>& filter,
                                         std::optional<bool> includeAllNetworks,
                                         const QString& thirdPartyInstanceId)
    : BaseJob(HttpVerb::Post, u"QueryPublicRoomsJob"_s,
              makePath("/_matrix/client/v3", "/publicRooms"), queryToQueryPublicRooms(server))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "limit"_L1, limit);
    addParam<IfNotEmpty>(_dataJson, "since"_L1, since);
    addParam<IfNotEmpty>(_dataJson, "filter"_L1, filter);
    addParam<IfNotEmpty>(_dataJson, "include_all_networks"_L1, includeAllNetworks);
    addParam<IfNotEmpty>(_dataJson, "third_party_instance_id"_L1, thirdPartyInstanceId);
    setRequestData({ _dataJson });
    addExpectedKey("chunk");
}

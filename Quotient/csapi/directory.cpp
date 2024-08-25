// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "directory.h"

using namespace Quotient;

SetRoomAliasJob::SetRoomAliasJob(const QString& roomAlias, const QString& roomId)
    : BaseJob(HttpVerb::Put, u"SetRoomAliasJob"_s,
              makePath("/_matrix/client/v3", "/directory/room/", roomAlias))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "room_id"_L1, roomId);
    setRequestData({ _dataJson });
}

QUrl GetRoomIdByAliasJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomAlias)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/directory/room/", roomAlias));
}

GetRoomIdByAliasJob::GetRoomIdByAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Get, u"GetRoomIdByAliasJob"_s,
              makePath("/_matrix/client/v3", "/directory/room/", roomAlias), false)
{}

QUrl DeleteRoomAliasJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomAlias)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/directory/room/", roomAlias));
}

DeleteRoomAliasJob::DeleteRoomAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Delete, u"DeleteRoomAliasJob"_s,
              makePath("/_matrix/client/v3", "/directory/room/", roomAlias))
{}

QUrl GetLocalAliasesJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/rooms/", roomId, "/aliases"));
}

GetLocalAliasesJob::GetLocalAliasesJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, u"GetLocalAliasesJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/aliases"))
{
    addExpectedKey("aliases");
}

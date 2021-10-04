/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "directory.h"

using namespace Quotient;

SetRoomAliasJob::SetRoomAliasJob(const QString& roomAlias, const QString& roomId)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetRoomAliasJob"),
              makePath("/_matrix/client/r0", "/directory/room/", roomAlias))
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("room_id"), roomId);
    setRequestData(std::move(_data));
}

QUrl GetRoomIdByAliasJob::makeRequestUrl(QUrl baseUrl, const QString& roomAlias)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/r0",
                                            "/directory/room/", roomAlias));
}

GetRoomIdByAliasJob::GetRoomIdByAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomIdByAliasJob"),
              makePath("/_matrix/client/r0", "/directory/room/", roomAlias),
              false)
{}

QUrl DeleteRoomAliasJob::makeRequestUrl(QUrl baseUrl, const QString& roomAlias)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/r0",
                                            "/directory/room/", roomAlias));
}

DeleteRoomAliasJob::DeleteRoomAliasJob(const QString& roomAlias)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteRoomAliasJob"),
              makePath("/_matrix/client/r0", "/directory/room/", roomAlias))
{}

QUrl GetLocalAliasesJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/r0", "/rooms/",
                                            roomId, "/aliases"));
}

GetLocalAliasesJob::GetLocalAliasesJob(const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetLocalAliasesJob"),
              makePath("/_matrix/client/r0", "/rooms/", roomId, "/aliases"))
{
    addExpectedKey("aliases");
}

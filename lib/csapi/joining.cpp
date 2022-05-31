/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "joining.h"

using namespace Quotient;

JoinRoomByIdJob::JoinRoomByIdJob(
    const QString& roomId, const Omittable<ThirdPartySigned>& thirdPartySigned,
    const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("JoinRoomByIdJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/join"))
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_signed"),
                         thirdPartySigned);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
    addExpectedKey("room_id");
}

auto queryToJoinRoom(const QStringList& serverName)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("server_name"), serverName);
    return _q;
}

JoinRoomJob::JoinRoomJob(const QString& roomIdOrAlias,
                         const QStringList& serverName,
                         const Omittable<ThirdPartySigned>& thirdPartySigned,
                         const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("JoinRoomJob"),
              makePath("/_matrix/client/v3", "/join/", roomIdOrAlias),
              queryToJoinRoom(serverName))
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_signed"),
                         thirdPartySigned);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
    addExpectedKey("room_id");
}

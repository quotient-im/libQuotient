/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "joining.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

JoinRoomByIdJob::JoinRoomByIdJob(
    const QString& roomId, const Omittable<ThirdPartySigned>& thirdPartySigned,
    const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("JoinRoomByIdJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId % "/join")
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
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("server_name"), serverName);
    return _q;
}

JoinRoomJob::JoinRoomJob(const QString& roomIdOrAlias,
                         const QStringList& serverName,
                         const Omittable<ThirdPartySigned>& thirdPartySigned,
                         const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("JoinRoomJob"),
              QStringLiteral("/_matrix/client/r0") % "/join/" % roomIdOrAlias,
              queryToJoinRoom(serverName))
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("third_party_signed"),
                         thirdPartySigned);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
    addExpectedKey("room_id");
}

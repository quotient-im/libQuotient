// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "joining.h"

using namespace Quotient;

JoinRoomByIdJob::JoinRoomByIdJob(const QString& roomId,
                                 const std::optional<ThirdPartySigned>& thirdPartySigned,
                                 const QString& reason)
    : BaseJob(HttpVerb::Post, u"JoinRoomByIdJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/join"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "third_party_signed"_L1, thirdPartySigned);
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
    addExpectedKey("room_id");
}

auto queryToJoinRoom(const QStringList& serverName)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"server_name"_s, serverName);
    return _q;
}

JoinRoomJob::JoinRoomJob(const QString& roomIdOrAlias, const QStringList& serverName,
                         const std::optional<ThirdPartySigned>& thirdPartySigned,
                         const QString& reason)
    : BaseJob(HttpVerb::Post, u"JoinRoomJob"_s,
              makePath("/_matrix/client/v3", "/join/", roomIdOrAlias), queryToJoinRoom(serverName))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "third_party_signed"_L1, thirdPartySigned);
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
    addExpectedKey("room_id");
}

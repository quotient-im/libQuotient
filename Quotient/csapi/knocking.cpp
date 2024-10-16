// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "knocking.h"

using namespace Quotient;

auto queryToKnockRoom(const QStringList& serverName, const QStringList& via)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"server_name"_s, serverName);
    addParam<IfNotEmpty>(_q, u"via"_s, via);
    return _q;
}

KnockRoomJob::KnockRoomJob(const QString& roomIdOrAlias, const QStringList& serverName,
                           const QStringList& via, const QString& reason)
    : BaseJob(HttpVerb::Post, u"KnockRoomJob"_s,
              makePath("/_matrix/client/v3", "/knock/", roomIdOrAlias),
              queryToKnockRoom(serverName, via))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
    addExpectedKey(u"room_id"_s);
}

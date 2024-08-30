// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "kicking.h"

using namespace Quotient;

KickJob::KickJob(const QString& roomId, const QString& userId, const QString& reason)
    : BaseJob(HttpVerb::Post, u"KickJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/kick"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "user_id"_L1, userId);
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
}

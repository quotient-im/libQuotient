// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "banning.h"

using namespace Quotient;

BanJob::BanJob(const QString& roomId, const QString& userId, const QString& reason)
    : BaseJob(HttpVerb::Post, u"BanJob"_s, makePath("/_matrix/client/v3", "/rooms/", roomId, "/ban"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "user_id"_L1, userId);
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
}

UnbanJob::UnbanJob(const QString& roomId, const QString& userId, const QString& reason)
    : BaseJob(HttpVerb::Post, u"UnbanJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/unban"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "user_id"_L1, userId);
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
}

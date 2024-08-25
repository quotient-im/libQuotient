// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "inviting.h"

using namespace Quotient;

InviteUserJob::InviteUserJob(const QString& roomId, const QString& userId, const QString& reason)
    : BaseJob(HttpVerb::Post, u"InviteUserJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/invite"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "user_id"_L1, userId);
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
}

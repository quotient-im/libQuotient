// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "leaving.h"

using namespace Quotient;

LeaveRoomJob::LeaveRoomJob(const QString& roomId, const QString& reason)
    : BaseJob(HttpVerb::Post, u"LeaveRoomJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/leave"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
}

QUrl ForgetRoomJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/rooms/", roomId, "/forget"));
}

ForgetRoomJob::ForgetRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, u"ForgetRoomJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/forget"))
{}

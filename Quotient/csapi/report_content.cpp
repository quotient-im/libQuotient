// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "report_content.h"

using namespace Quotient;

ReportRoomJob::ReportRoomJob(const QString &roomId, const QString &reason)
    : BaseJob(HttpVerb::Post, u"ReportRoomJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/report"))
{
    QJsonObject _dataJson;
    addParam(_dataJson, "reason"_L1, reason);
    setRequestData({_dataJson});
}

ReportEventJob::ReportEventJob(const QString &roomId, const QString &eventId,
                               std::optional<int> score, const QString &reason)
    : BaseJob(HttpVerb::Post, u"ReportEventJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/report/", eventId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "score"_L1, score);
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({_dataJson});
}

ReportUserJob::ReportUserJob(const UserId &userId, const QString &reason)
    : BaseJob(HttpVerb::Post, u"ReportUserJob"_s,
              makePath("/_matrix/client/v3", "/users/", userId, "/report"))
{
    QJsonObject _dataJson;
    addParam(_dataJson, "reason"_L1, reason);
    setRequestData({_dataJson});
}

// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "report_content.h"

using namespace Quotient;

ReportContentJob::ReportContentJob(const QString& roomId, const QString& eventId,
                                   std::optional<int> score, const QString& reason)
    : BaseJob(HttpVerb::Post, u"ReportContentJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/report/", eventId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "score"_L1, score);
    addParam<IfNotEmpty>(_dataJson, "reason"_L1, reason);
    setRequestData({ _dataJson });
}

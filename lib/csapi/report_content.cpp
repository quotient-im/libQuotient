/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "report_content.h"

using namespace Quotient;

ReportContentJob::ReportContentJob(const QString& roomId, const QString& eventId,
                                   Omittable<int> score, const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("ReportContentJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/report/",
                       eventId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("score"), score);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("reason"), reason);
    setRequestData({ _dataJson });
}

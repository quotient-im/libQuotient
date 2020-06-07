/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "report_content.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

ReportContentJob::ReportContentJob(const QString& roomId, const QString& eventId,
                                   int score, const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("ReportContentJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/report/" % eventId)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("score"), score);
    addParam<>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
}

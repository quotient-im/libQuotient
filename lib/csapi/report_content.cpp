/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "report_content.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto ReportContentJobName = QStringLiteral("ReportContentJob");

ReportContentJob::ReportContentJob(const QString& roomId, const QString& eventId,
                                   int score, const QString& reason)
    : BaseJob(HttpVerb::Post, ReportContentJobName,
              basePath % "/rooms/" % roomId % "/report/" % eventId)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("score"), score);
    addParam<>(_data, QStringLiteral("reason"), reason);
    setRequestData(_data);
}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "report_content.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

ReportContentJob::ReportContentJob(const QString& roomId, const QString& eventId, int score, const QString& reason)
    : BaseJob(HttpVerb::Post, "ReportContentJob",
        basePath % "/rooms/" % roomId % "/report/" % eventId)
{
    QJsonObject _data;
    addToJson<>(_data, "score", score);
    addToJson<>(_data, "reason", reason);
    setRequestData(_data);
}


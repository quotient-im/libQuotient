/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "typing.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto SetTypingJobName = QStringLiteral("SetTypingJob");

SetTypingJob::SetTypingJob(const QString& userId, const QString& roomId,
                           bool typing, Omittable<int> timeout)
    : BaseJob(HttpVerb::Put, SetTypingJobName,
              basePath % "/rooms/" % roomId % "/typing/" % userId)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("typing"), typing);
    addParam<IfNotEmpty>(_data, QStringLiteral("timeout"), timeout);
    setRequestData(_data);
}

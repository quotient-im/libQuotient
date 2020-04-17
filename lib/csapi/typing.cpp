/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "typing.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

SetTypingJob::SetTypingJob(const QString& userId, const QString& roomId,
                           bool typing, Omittable<int> timeout)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetTypingJob"),
              basePath % "/rooms/" % roomId % "/typing/" % userId)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("typing"), typing);
    addParam<IfNotEmpty>(_data, QStringLiteral("timeout"), timeout);
    setRequestData(_data);
}

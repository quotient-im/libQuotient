/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "typing.h"

using namespace Quotient;

SetTypingJob::SetTypingJob(const QString& userId, const QString& roomId,
                           bool typing, Omittable<int> timeout)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetTypingJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/typing/",
                       userId))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("typing"), typing);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("timeout"), timeout);
    setRequestData({ _dataJson });
}

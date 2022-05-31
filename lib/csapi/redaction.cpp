/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "redaction.h"

using namespace Quotient;

RedactEventJob::RedactEventJob(const QString& roomId, const QString& eventId,
                               const QString& txnId, const QString& reason)
    : BaseJob(HttpVerb::Put, QStringLiteral("RedactEventJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/redact/",
                       eventId, "/", txnId))
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
}

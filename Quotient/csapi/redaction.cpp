// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "redaction.h"

using namespace Quotient;

RedactEventJob::RedactEventJob(const QString& roomId, const QString& eventId, const QString& txnId,
                               const QString& reason)
    : BaseJob(HttpVerb::Put, QStringLiteral("RedactEventJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/redact/", eventId, "/", txnId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("reason"), reason);
    setRequestData({ _dataJson });
}

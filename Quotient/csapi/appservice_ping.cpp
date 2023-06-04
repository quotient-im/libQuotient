/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "appservice_ping.h"

using namespace Quotient;

PingAppserviceJob::PingAppserviceJob(const QString& appserviceId,
                                     const QString& transactionId)
    : BaseJob(HttpVerb::Post, QStringLiteral("PingAppserviceJob"),
              makePath("/_matrix/client/v1", "/appservice/", appserviceId,
                       "/ping"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("transaction_id"),
                         transactionId);
    setRequestData({ _dataJson });
    addExpectedKey("duration_ms");
}

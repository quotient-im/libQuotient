/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "receipts.h"

using namespace Quotient;

PostReceiptJob::PostReceiptJob(const QString& roomId, const QString& receiptType,
                               const QString& eventId, const QString& threadId)
    : BaseJob(HttpVerb::Post, QStringLiteral("PostReceiptJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/receipt/",
                       receiptType, "/", eventId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("thread_id"), threadId);
    setRequestData({ _dataJson });
}

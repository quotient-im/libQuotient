// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "receipts.h"

using namespace Quotient;

PostReceiptJob::PostReceiptJob(const QString& roomId, const QString& receiptType,
                               const QString& eventId, const QString& threadId)
    : BaseJob(HttpVerb::Post, u"PostReceiptJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/receipt/", receiptType, "/",
                       eventId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "thread_id"_L1, threadId);
    setRequestData({ _dataJson });
}

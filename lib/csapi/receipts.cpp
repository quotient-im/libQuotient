/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "receipts.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

PostReceiptJob::PostReceiptJob(const QString& roomId, const QString& receiptType,
                               const QString& eventId,
                               const QJsonObject& receipt)
    : BaseJob(HttpVerb::Post, QStringLiteral("PostReceiptJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/receipt/" % receiptType % "/" % eventId)
{
    setRequestData(Data(toJson(receipt)));
}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "receipts.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

PostReceiptJob::PostReceiptJob(const QString& roomId, const QString& receiptType, const QString& eventId, const QJsonObject& receipt)
    : BaseJob(HttpVerb::Post, "PostReceiptJob",
        basePath % "/rooms/" % roomId % "/receipt/" % receiptType % "/" % eventId)
{
    setRequestData(Data(toJson(receipt)));
}


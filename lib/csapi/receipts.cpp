/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "receipts.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto PostReceiptJobName = QStringLiteral("PostReceiptJob");

PostReceiptJob::PostReceiptJob(const QString& roomId,
                               const QString& receiptType,
                               const QString& eventId,
                               const QJsonObject& receipt)
    : BaseJob(HttpVerb::Post, PostReceiptJobName,
              basePath % "/rooms/" % roomId % "/receipt/" % receiptType % "/"
                      % eventId)
{
    setRequestData(Data(toJson(receipt)));
}

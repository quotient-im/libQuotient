/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "to_device.h"

using namespace Quotient;

SendToDeviceJob::SendToDeviceJob(
    const QString& eventType, const QString& txnId,
    const QHash<QString, QHash<QString, QJsonObject>>& messages)
    : BaseJob(HttpVerb::Put, QStringLiteral("SendToDeviceJob"),
              makePath("/_matrix/client/v3", "/sendToDevice/", eventType, "/",
                       txnId))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("messages"), messages);
    setRequestData({ _dataJson });
}

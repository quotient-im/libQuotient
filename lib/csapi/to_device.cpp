/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "to_device.h"

using namespace Quotient;

SendToDeviceJob::SendToDeviceJob(
    const QString& eventType, const QString& txnId,
    const QHash<QString, QHash<QString, QJsonObject>>& messages)
    : BaseJob(HttpVerb::Put, QStringLiteral("SendToDeviceJob"),
              makePath("/_matrix/client/r0", "/sendToDevice/", eventType, "/",
                       txnId))
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("messages"), messages);
    setRequestData(std::move(_data));
}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "to_device.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

SendToDeviceJob::SendToDeviceJob(
    const QString& eventType, const QString& txnId,
    const QHash<QString, QHash<QString, QJsonObject>>& messages)
    : BaseJob(HttpVerb::Put, QStringLiteral("SendToDeviceJob"),
              QStringLiteral("/_matrix/client/r0") % "/sendToDevice/"
                  % eventType % "/" % txnId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("messages"), messages);
    setRequestData(std::move(_data));
}

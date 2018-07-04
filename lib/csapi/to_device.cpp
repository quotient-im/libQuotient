/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "to_device.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto SendToDeviceJobName = QStringLiteral("SendToDeviceJob");

SendToDeviceJob::SendToDeviceJob(const QString& eventType, const QString& txnId, const QHash<QString, QHash<QString, QJsonObject>>& messages)
    : BaseJob(HttpVerb::Put, SendToDeviceJobName,
        basePath % "/sendToDevice/" % eventType % "/" % txnId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("messages"), messages);
    setRequestData(_data);
}


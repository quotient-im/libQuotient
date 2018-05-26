/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "to_device.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

SendToDeviceJob::SendToDeviceJob(const QString& eventType, const QString& txnId, const QHash<QString, QHash<QString, QJsonObject>>& messages)
    : BaseJob(HttpVerb::Put, "SendToDeviceJob",
        basePath % "/sendToDevice/" % eventType % "/" % txnId)
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "messages", messages);
    setRequestData(_data);
}


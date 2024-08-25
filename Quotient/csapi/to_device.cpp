// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "to_device.h"

using namespace Quotient;

SendToDeviceJob::SendToDeviceJob(const QString& eventType, const QString& txnId,
                                 const QHash<UserId, QHash<QString, QJsonObject>>& messages)
    : BaseJob(HttpVerb::Put, u"SendToDeviceJob"_s,
              makePath("/_matrix/client/v3", "/sendToDevice/", eventType, "/", txnId))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "messages"_L1, messages);
    setRequestData({ _dataJson });
}

// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "typing.h"

using namespace Quotient;

SetTypingJob::SetTypingJob(const QString& userId, const QString& roomId, bool typing,
                           std::optional<int> timeout)
    : BaseJob(HttpVerb::Put, u"SetTypingJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/typing/", userId))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "typing"_L1, typing);
    addParam<IfNotEmpty>(_dataJson, "timeout"_L1, timeout);
    setRequestData({ _dataJson });
}

// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "read_markers.h"

using namespace Quotient;

SetReadMarkerJob::SetReadMarkerJob(const QString& roomId, const QString& mFullyRead,
                                   const QString& mRead, const QString& mReadPrivate)
    : BaseJob(HttpVerb::Post, u"SetReadMarkerJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/read_markers"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "m.fully_read"_L1, mFullyRead);
    addParam<IfNotEmpty>(_dataJson, "m.read"_L1, mRead);
    addParam<IfNotEmpty>(_dataJson, "m.read.private"_L1, mReadPrivate);
    setRequestData({ _dataJson });
}

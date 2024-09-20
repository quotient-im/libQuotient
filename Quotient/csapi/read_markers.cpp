// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "read_markers.h"

using namespace Quotient;

SetReadMarkerJob::SetReadMarkerJob(const QString& roomId, const QString& fullyRead,
                                   const QString& read, const QString& readPrivate)
    : BaseJob(HttpVerb::Post, u"SetReadMarkerJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/read_markers"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "m.fully_read"_L1, fullyRead);
    addParam<IfNotEmpty>(_dataJson, "m.read"_L1, read);
    addParam<IfNotEmpty>(_dataJson, "m.read.private"_L1, readPrivate);
    setRequestData({ _dataJson });
}

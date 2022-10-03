/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "read_markers.h"

using namespace Quotient;

SetReadMarkerJob::SetReadMarkerJob(const QString& roomId,
                                   const QString& mFullyRead,
                                   const QString& mRead,
                                   const QString& mReadPrivate)
    : BaseJob(HttpVerb::Post, QStringLiteral("SetReadMarkerJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/read_markers"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("m.fully_read"), mFullyRead);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("m.read"), mRead);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("m.read.private"),
                         mReadPrivate);
    setRequestData({ _dataJson });
}

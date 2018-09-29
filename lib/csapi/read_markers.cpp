/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "read_markers.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto SetReadMarkerJobName = QStringLiteral("SetReadMarkerJob");

SetReadMarkerJob::SetReadMarkerJob(const QString& roomId, const QString& mFullyRead, const QString& mRead)
    : BaseJob(HttpVerb::Post, SetReadMarkerJobName,
        basePath % "/rooms/" % roomId % "/read_markers")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("m.fully_read"), mFullyRead);
    addParam<IfNotEmpty>(_data, QStringLiteral("m.read"), mRead);
    setRequestData(_data);
}


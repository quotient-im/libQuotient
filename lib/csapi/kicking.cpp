/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "kicking.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

KickJob::KickJob(const QString& roomId, const QString& userId, const QString& reason)
    : BaseJob(HttpVerb::Post, "KickJob",
        basePath % "/rooms/" % roomId % "/kick")
{
    QJsonObject _data;
    addToJson<>(_data, "user_id", userId);
    addToJson<IfNotEmpty>(_data, "reason", reason);
    setRequestData(_data);
}


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
        basePath % "/rooms/" % roomId % "/kick",
        Query { }
    )
{
    QJsonObject _data;
    _data.insert("user_id", toJson(userId));
    if (!reason.isEmpty())
        _data.insert("reason", toJson(reason));
    setRequestData(_data);
}


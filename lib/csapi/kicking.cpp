/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "kicking.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto KickJobName = QStringLiteral("KickJob");

KickJob::KickJob(const QString& roomId, const QString& userId, const QString& reason)
    : BaseJob(HttpVerb::Post, KickJobName,
        basePath % "/rooms/" % roomId % "/kick")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(_data);
}


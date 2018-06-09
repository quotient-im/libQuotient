/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "banning.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

BanJob::BanJob(const QString& roomId, const QString& userId, const QString& reason)
    : BaseJob(HttpVerb::Post, "BanJob",
        basePath % "/rooms/" % roomId % "/ban")
{
    QJsonObject _data;
    addParam<>(_data, "user_id", userId);
    addParam<IfNotEmpty>(_data, "reason", reason);
    setRequestData(_data);
}

UnbanJob::UnbanJob(const QString& roomId, const QString& userId)
    : BaseJob(HttpVerb::Post, "UnbanJob",
        basePath % "/rooms/" % roomId % "/unban")
{
    QJsonObject _data;
    addParam<>(_data, "user_id", userId);
    setRequestData(_data);
}


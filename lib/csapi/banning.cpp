/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "banning.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto BanJobName = QStringLiteral("BanJob");

BanJob::BanJob(const QString& roomId, const QString& userId,
               const QString& reason)
    : BaseJob(HttpVerb::Post, BanJobName, basePath % "/rooms/" % roomId % "/ban")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(_data);
}

static const auto UnbanJobName = QStringLiteral("UnbanJob");

UnbanJob::UnbanJob(const QString& roomId, const QString& userId)
    : BaseJob(HttpVerb::Post, UnbanJobName,
              basePath % "/rooms/" % roomId % "/unban")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    setRequestData(_data);
}

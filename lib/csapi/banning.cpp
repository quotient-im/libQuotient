/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "banning.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

BanJob::BanJob(const QString& roomId, const QString& userId,
               const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("BanJob"),
              basePath % "/rooms/" % roomId % "/ban")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(_data);
}

UnbanJob::UnbanJob(const QString& roomId, const QString& userId)
    : BaseJob(HttpVerb::Post, QStringLiteral("UnbanJob"),
              basePath % "/rooms/" % roomId % "/unban")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    setRequestData(_data);
}

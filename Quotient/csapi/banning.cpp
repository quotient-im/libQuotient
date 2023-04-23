/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "banning.h"

using namespace Quotient;

BanJob::BanJob(const QString& roomId, const QString& userId,
               const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("BanJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/ban"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("reason"), reason);
    setRequestData({ _dataJson });
}

UnbanJob::UnbanJob(const QString& roomId, const QString& userId,
                   const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("UnbanJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/unban"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("reason"), reason);
    setRequestData({ _dataJson });
}

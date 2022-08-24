/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "kicking.h"

using namespace Quotient;

KickJob::KickJob(const QString& roomId, const QString& userId,
                 const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("KickJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/kick"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("reason"), reason);
    setRequestData({ _dataJson });
}

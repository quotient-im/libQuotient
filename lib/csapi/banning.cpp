/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "banning.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

BanJob::BanJob(const QString& roomId, const QString& userId,
               const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("BanJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId % "/ban")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
}

UnbanJob::UnbanJob(const QString& roomId, const QString& userId,
                   const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("UnbanJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/unban")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
}

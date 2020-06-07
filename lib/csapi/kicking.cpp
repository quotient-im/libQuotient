/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "kicking.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

KickJob::KickJob(const QString& roomId, const QString& userId,
                 const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("KickJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId % "/kick")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "inviting.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

InviteUserJob::InviteUserJob(const QString& roomId, const QString& userId)
    : BaseJob(HttpVerb::Post, QStringLiteral("InviteUserJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/invite")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    setRequestData(std::move(_data));
}

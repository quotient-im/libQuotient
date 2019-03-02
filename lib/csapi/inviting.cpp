/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "inviting.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

static const auto InviteUserJobName = QStringLiteral("InviteUserJob");

InviteUserJob::InviteUserJob(const QString& roomId, const QString& userId)
    : BaseJob(HttpVerb::Post, InviteUserJobName,
              basePath % "/rooms/" % roomId % "/invite")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("user_id"), userId);
    setRequestData(_data);
}

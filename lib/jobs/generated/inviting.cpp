/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "inviting.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

InviteUserJob::InviteUserJob(const QString& roomId, const QString& userId)
    : BaseJob(HttpVerb::Post, "InviteUserJob",
        basePath % "/rooms/" % roomId % "/invite")
{
    QJsonObject _data;
    _data.insert("user_id", toJson(userId));
    setRequestData(_data);
}


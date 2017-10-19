/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "kicking.h"

#include "jobs/converters.h"
#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

KickJob::KickJob(QString roomId, QString user_id, QString reason)
    : BaseJob(HttpVerb::Post, "KickJob",
        basePath % "/rooms/" % roomId % "/kick",
        Query { }
    )
{
    Data _data;
    _data.insert("user_id", toJson(user_id));
    if (!reason.isEmpty())
        _data.insert("reason", toJson(reason));
    setRequestData(_data);
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "banning.h"

#include "converters.h"
#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

BanJob::BanJob(QString roomId, QString userId, QString reason)
    : BaseJob(HttpVerb::Post, "BanJob",
        basePath % "/rooms/" % roomId % "/ban",
        Query { }
    )
{
    Data _data;
    _data.insert("user_id", toJson(userId));
    if (!reason.isEmpty())
        _data.insert("reason", toJson(reason));
    setRequestData(_data);
}

UnbanJob::UnbanJob(QString roomId, QString userId)
    : BaseJob(HttpVerb::Post, "UnbanJob",
        basePath % "/rooms/" % roomId % "/unban",
        Query { }
    )
{
    Data _data;
    _data.insert("user_id", toJson(userId));
    setRequestData(_data);
}


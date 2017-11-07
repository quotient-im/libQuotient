/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "banning.h"

#include "converters.h"
#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

BanJob::BanJob(QString roomId, QString user_id, QString reason)
    : BaseJob(HttpVerb::Post, "BanJob",
        basePath % "/rooms/" % roomId % "/ban",
        Query { },
        Data {
            { "user_id", toJson(user_id) },
            { "reason", toJson(reason) }
        }
    )
{ }

UnbanJob::UnbanJob(QString roomId, QString user_id)
    : BaseJob(HttpVerb::Post, "UnbanJob",
        basePath % "/rooms/" % roomId % "/unban",
        Query { },
        Data {
            { "user_id", toJson(user_id) }
        }
    )
{ }


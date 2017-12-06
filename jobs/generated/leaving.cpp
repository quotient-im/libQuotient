/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */


#include "leaving.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

LeaveRoomJob::LeaveRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, "LeaveRoomJob",
        basePath % "/rooms/" % roomId % "/leave",
        Query { }
    )
{ }

ForgetRoomJob::ForgetRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, "ForgetRoomJob",
        basePath % "/rooms/" % roomId % "/forget",
        Query { }
    )
{ }


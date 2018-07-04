/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "leaving.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

QUrl LeaveRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/rooms/" % roomId % "/leave");
}

static const auto LeaveRoomJobName = QStringLiteral("LeaveRoomJob");

LeaveRoomJob::LeaveRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, LeaveRoomJobName,
        basePath % "/rooms/" % roomId % "/leave")
{
}

QUrl ForgetRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/rooms/" % roomId % "/forget");
}

static const auto ForgetRoomJobName = QStringLiteral("ForgetRoomJob");

ForgetRoomJob::ForgetRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, ForgetRoomJobName,
        basePath % "/rooms/" % roomId % "/forget")
{
}


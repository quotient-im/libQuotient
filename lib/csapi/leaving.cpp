/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "leaving.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

QUrl LeaveRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/rooms/" % roomId % "/leave");
}

LeaveRoomJob::LeaveRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, QStringLiteral("LeaveRoomJob"),
              basePath % "/rooms/" % roomId % "/leave")
{}

QUrl ForgetRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/rooms/" % roomId % "/forget");
}

ForgetRoomJob::ForgetRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, QStringLiteral("ForgetRoomJob"),
              basePath % "/rooms/" % roomId % "/forget")
{}

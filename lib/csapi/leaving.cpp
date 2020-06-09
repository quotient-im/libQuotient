/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "leaving.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl LeaveRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/rooms/" % roomId % "/leave");
}

LeaveRoomJob::LeaveRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, QStringLiteral("LeaveRoomJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/leave")
{}

QUrl ForgetRoomJob::makeRequestUrl(QUrl baseUrl, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/rooms/" % roomId % "/forget");
}

ForgetRoomJob::ForgetRoomJob(const QString& roomId)
    : BaseJob(HttpVerb::Post, QStringLiteral("ForgetRoomJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/forget")
{}

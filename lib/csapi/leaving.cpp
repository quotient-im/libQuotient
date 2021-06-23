/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "leaving.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

LeaveRoomJob::LeaveRoomJob(const QString& roomId, const QString& reason)
    : BaseJob(HttpVerb::Post, QStringLiteral("LeaveRoomJob"),
              QStringLiteral("/_matrix/client/r0") % "/rooms/" % roomId
                  % "/leave")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("reason"), reason);
    setRequestData(std::move(_data));
}

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

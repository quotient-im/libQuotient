/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "presence.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

SetPresenceJob::SetPresenceJob(const QString& userId, const QString& presence,
                               const QString& statusMsg)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetPresenceJob"),
              QStringLiteral("/_matrix/client/r0") % "/presence/" % userId
                  % "/status")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("presence"), presence);
    addParam<IfNotEmpty>(_data, QStringLiteral("status_msg"), statusMsg);
    setRequestData(std::move(_data));
}

QUrl GetPresenceJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/presence/" % userId % "/status");
}

GetPresenceJob::GetPresenceJob(const QString& userId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPresenceJob"),
              QStringLiteral("/_matrix/client/r0") % "/presence/" % userId
                  % "/status")
{
    addExpectedKey("presence");
}

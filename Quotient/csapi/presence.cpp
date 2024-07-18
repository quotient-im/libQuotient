// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "presence.h"

using namespace Quotient;

SetPresenceJob::SetPresenceJob(const QString& userId, const QString& presence,
                               const QString& statusMsg)
    : BaseJob(HttpVerb::Put, QStringLiteral("SetPresenceJob"),
              makePath("/_matrix/client/v3", "/presence/", userId, "/status"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("presence"), presence);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("status_msg"), statusMsg);
    setRequestData({ _dataJson });
}

QUrl GetPresenceJob::makeRequestUrl(const HomeserverData& hsData, const QString& userId)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/presence/", userId, "/status"));
}

GetPresenceJob::GetPresenceJob(const QString& userId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetPresenceJob"),
              makePath("/_matrix/client/v3", "/presence/", userId, "/status"))
{
    addExpectedKey("presence");
}

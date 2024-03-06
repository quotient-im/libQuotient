// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "third_party_membership.h"

using namespace Quotient;

InviteBy3PIDJob::InviteBy3PIDJob(const QString& roomId, const QString& idServer,
                                 const QString& idAccessToken, const QString& medium,
                                 const QString& address)
    : BaseJob(HttpVerb::Post, QStringLiteral("InviteBy3PIDJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/invite"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("id_server"), idServer);
    addParam<>(_dataJson, QStringLiteral("id_access_token"), idAccessToken);
    addParam<>(_dataJson, QStringLiteral("medium"), medium);
    addParam<>(_dataJson, QStringLiteral("address"), address);
    setRequestData({ _dataJson });
}

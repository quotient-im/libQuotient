/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "third_party_membership.h"

using namespace Quotient;

InviteBy3PIDJob::InviteBy3PIDJob(const QString& roomId, const QString& idServer,
                                 const QString& idAccessToken,
                                 const QString& medium, const QString& address)
    : BaseJob(HttpVerb::Post, QStringLiteral("InviteBy3PIDJob"),
              makePath("/_matrix/client/r0", "/rooms/", roomId, "/invite"))
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("id_server"), idServer);
    addParam<>(_data, QStringLiteral("id_access_token"), idAccessToken);
    addParam<>(_data, QStringLiteral("medium"), medium);
    addParam<>(_data, QStringLiteral("address"), address);
    setRequestData(std::move(_data));
}

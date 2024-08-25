// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "third_party_membership.h"

using namespace Quotient;

InviteBy3PIDJob::InviteBy3PIDJob(const QString& roomId, const QString& idServer,
                                 const QString& idAccessToken, const QString& medium,
                                 const QString& address)
    : BaseJob(HttpVerb::Post, u"InviteBy3PIDJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/invite"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "id_server"_L1, idServer);
    addParam<>(_dataJson, "id_access_token"_L1, idAccessToken);
    addParam<>(_dataJson, "medium"_L1, medium);
    addParam<>(_dataJson, "address"_L1, address);
    setRequestData({ _dataJson });
}

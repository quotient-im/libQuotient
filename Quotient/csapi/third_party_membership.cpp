// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "third_party_membership.h"

using namespace Quotient;

InviteBy3PIDJob::InviteBy3PIDJob(const QString &roomId, const Invite3pid &data)
    : BaseJob(HttpVerb::Post, u"InviteBy3PIDJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/invite"))
{
    setRequestData({toJson(data)});
}

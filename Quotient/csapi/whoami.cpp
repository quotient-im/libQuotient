// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "whoami.h"

using namespace Quotient;

QUrl GetTokenOwnerJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/account/whoami"));
}

GetTokenOwnerJob::GetTokenOwnerJob()
    : BaseJob(HttpVerb::Get, u"GetTokenOwnerJob"_s,
              makePath("/_matrix/client/v3", "/account/whoami"))
{
    addExpectedKey(u"user_id"_s);
}

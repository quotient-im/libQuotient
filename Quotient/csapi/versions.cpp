// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "versions.h"

using namespace Quotient;

QUrl GetVersionsJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client", "/versions"));
}

GetVersionsJob::GetVersionsJob()
    : BaseJob(HttpVerb::Get, u"GetVersionsJob"_s, makePath("/_matrix/client", "/versions"))
{
    addExpectedKey(u"versions"_s);
}

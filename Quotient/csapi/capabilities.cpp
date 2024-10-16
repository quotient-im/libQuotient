// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "capabilities.h"

using namespace Quotient;

QUrl GetCapabilitiesJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/capabilities"));
}

GetCapabilitiesJob::GetCapabilitiesJob()
    : BaseJob(HttpVerb::Get, u"GetCapabilitiesJob"_s,
              makePath("/_matrix/client/v3", "/capabilities"))
{
    addExpectedKey(u"capabilities"_s);
}

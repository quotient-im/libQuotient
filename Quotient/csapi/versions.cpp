// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "versions.h"

using namespace Quotient;

QUrl GetVersionsJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client", "/versions"));
}

GetVersionsJob::GetVersionsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetVersionsJob"),
              makePath("/_matrix/client", "/versions"))
{
    addExpectedKey("versions");
}

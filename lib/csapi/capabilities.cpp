/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "capabilities.h"

using namespace Quotient;

QUrl GetCapabilitiesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl), makePath("/_matrix/client/r0", "/capabilities"));
}

GetCapabilitiesJob::GetCapabilitiesJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetCapabilitiesJob"),
              makePath("/_matrix/client/r0", "/capabilities"))
{
    addExpectedKey("capabilities");
}

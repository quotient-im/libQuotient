/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "capabilities.h"

using namespace Quotient;

QUrl GetCapabilitiesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl), makePath("/_matrix/client/v3", "/capabilities"));
}

GetCapabilitiesJob::GetCapabilitiesJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetCapabilitiesJob"),
              makePath("/_matrix/client/v3", "/capabilities"))
{
    addExpectedKey("capabilities");
}

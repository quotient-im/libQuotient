/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "versions.h"

using namespace Quotient;

QUrl GetVersionsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client", "/versions"));
}

GetVersionsJob::GetVersionsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetVersionsJob"),
              makePath("/_matrix/client", "/versions"), false)
{
    addExpectedKey("versions");
}

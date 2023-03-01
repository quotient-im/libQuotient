/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "whoami.h"

using namespace Quotient;

QUrl GetTokenOwnerJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl), makePath("/_matrix/client/v3", "/account/whoami"));
}

GetTokenOwnerJob::GetTokenOwnerJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetTokenOwnerJob"),
              makePath("/_matrix/client/v3", "/account/whoami"))
{
    addExpectedKey("user_id");
}

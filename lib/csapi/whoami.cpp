/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "whoami.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetTokenOwnerJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/account/whoami");
}

GetTokenOwnerJob::GetTokenOwnerJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetTokenOwnerJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/whoami")
{
    addExpectedKey("user_id");
}

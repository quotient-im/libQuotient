/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "versions.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetVersionsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client")
                                       % "/versions");
}

GetVersionsJob::GetVersionsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetVersionsJob"),
              QStringLiteral("/_matrix/client") % "/versions", false)
{
    addExpectedKey("versions");
}

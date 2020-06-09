/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "capabilities.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetCapabilitiesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/capabilities");
}

GetCapabilitiesJob::GetCapabilitiesJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetCapabilitiesJob"),
              QStringLiteral("/_matrix/client/r0") % "/capabilities")
{
    addExpectedKey("capabilities");
}

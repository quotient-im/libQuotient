/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "wellknown.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetWellknownJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/.well-known")
                                       % "/matrix/client");
}

GetWellknownJob::GetWellknownJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetWellknownJob"),
              QStringLiteral("/.well-known") % "/matrix/client", false)
{}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "admin.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetWhoIsJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/admin/whois/" % userId);
}

GetWhoIsJob::GetWhoIsJob(const QString& userId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetWhoIsJob"),
              QStringLiteral("/_matrix/client/r0") % "/admin/whois/" % userId)
{}

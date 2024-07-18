// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "admin.h"

using namespace Quotient;

QUrl GetWhoIsJob::makeRequestUrl(const HomeserverData& hsData, const QString& userId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/admin/whois/", userId));
}

GetWhoIsJob::GetWhoIsJob(const QString& userId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetWhoIsJob"),
              makePath("/_matrix/client/v3", "/admin/whois/", userId))
{}

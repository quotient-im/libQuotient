// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "support.h"

using namespace Quotient;

QUrl GetWellknownSupportJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/.well-known", "/matrix/support"));
}

GetWellknownSupportJob::GetWellknownSupportJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetWellknownSupportJob"),
              makePath("/.well-known", "/matrix/support"), false)
{}

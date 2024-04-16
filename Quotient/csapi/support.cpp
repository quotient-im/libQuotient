// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "support.h"

using namespace Quotient;

QUrl GetWellknownSupportJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/.well-known", "/matrix/support"));
}

GetWellknownSupportJob::GetWellknownSupportJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetWellknownSupportJob"),
              makePath("/.well-known", "/matrix/support"), false)
{}

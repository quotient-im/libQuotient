// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "wellknown.h"

using namespace Quotient;

QUrl GetWellknownJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/.well-known", "/matrix/client"));
}

GetWellknownJob::GetWellknownJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetWellknownJob"),
              makePath("/.well-known", "/matrix/client"), false)
{}

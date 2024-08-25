// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "wellknown.h"

using namespace Quotient;

QUrl GetWellknownJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/.well-known", "/matrix/client"));
}

GetWellknownJob::GetWellknownJob()
    : BaseJob(HttpVerb::Get, u"GetWellknownJob"_s, makePath("/.well-known", "/matrix/client"), false)
{}

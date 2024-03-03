// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "voip.h"

using namespace Quotient;

QUrl GetTurnServerJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/voip/turnServer"));
}

GetTurnServerJob::GetTurnServerJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetTurnServerJob"),
              makePath("/_matrix/client/v3", "/voip/turnServer"))
{}

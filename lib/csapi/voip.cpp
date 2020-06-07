/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "voip.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetTurnServerJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/voip/turnServer");
}

GetTurnServerJob::GetTurnServerJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetTurnServerJob"),
              QStringLiteral("/_matrix/client/r0") % "/voip/turnServer")
{}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "logout.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl LogoutJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/logout");
}

LogoutJob::LogoutJob()
    : BaseJob(HttpVerb::Post, QStringLiteral("LogoutJob"),
              QStringLiteral("/_matrix/client/r0") % "/logout")
{}

QUrl LogoutAllJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/logout/all");
}

LogoutAllJob::LogoutAllJob()
    : BaseJob(HttpVerb::Post, QStringLiteral("LogoutAllJob"),
              QStringLiteral("/_matrix/client/r0") % "/logout/all")
{}

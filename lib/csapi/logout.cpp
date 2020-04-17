/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "logout.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

QUrl LogoutJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/logout");
}

LogoutJob::LogoutJob()
    : BaseJob(HttpVerb::Post, QStringLiteral("LogoutJob"), basePath % "/logout")
{}

QUrl LogoutAllJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/logout/all");
}

LogoutAllJob::LogoutAllJob()
    : BaseJob(HttpVerb::Post, QStringLiteral("LogoutAllJob"),
              basePath % "/logout/all")
{}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sso_login_redirect.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

BaseJob::Query queryToRedirectToSSO(const QString& redirectUrl)
{
    BaseJob::Query _q;
    addParam<>(_q, QStringLiteral("redirectUrl"), redirectUrl);
    return _q;
}

QUrl RedirectToSSOJob::makeRequestUrl(QUrl baseUrl, const QString& redirectUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/login/sso/redirect",
                                   queryToRedirectToSSO(redirectUrl));
}

RedirectToSSOJob::RedirectToSSOJob(const QString& redirectUrl)
    : BaseJob(HttpVerb::Get, QStringLiteral("RedirectToSSOJob"),
              basePath % "/login/sso/redirect",
              queryToRedirectToSSO(redirectUrl), {}, false)

{}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sso_login_redirect.h"

using namespace Quotient;

auto queryToRedirectToSSO(const QString& redirectUrl)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("redirectUrl"), redirectUrl);
    return _q;
}

QUrl RedirectToSSOJob::makeRequestUrl(QUrl baseUrl, const QString& redirectUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/r0",
                                            "/login/sso/redirect"),
                                   queryToRedirectToSSO(redirectUrl));
}

RedirectToSSOJob::RedirectToSSOJob(const QString& redirectUrl)
    : BaseJob(HttpVerb::Get, QStringLiteral("RedirectToSSOJob"),
              makePath("/_matrix/client/r0", "/login/sso/redirect"),
              queryToRedirectToSSO(redirectUrl), {}, false)
{}

auto queryToRedirectToIdP(const QString& redirectUrl)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("redirectUrl"), redirectUrl);
    return _q;
}

QUrl RedirectToIdPJob::makeRequestUrl(QUrl baseUrl, const QString& idpId,
                                      const QString& redirectUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/r0",
                                            "/login/sso/redirect/", idpId),
                                   queryToRedirectToIdP(redirectUrl));
}

RedirectToIdPJob::RedirectToIdPJob(const QString& idpId,
                                   const QString& redirectUrl)
    : BaseJob(HttpVerb::Get, QStringLiteral("RedirectToIdPJob"),
              makePath("/_matrix/client/r0", "/login/sso/redirect/", idpId),
              queryToRedirectToIdP(redirectUrl), {}, false)
{}

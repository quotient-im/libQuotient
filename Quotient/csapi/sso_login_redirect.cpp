// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "sso_login_redirect.h"

using namespace Quotient;

auto queryToRedirectToSSO(const QString& redirectUrl)
{
    QUrlQuery _q;
    addParam<>(_q, u"redirectUrl"_s, redirectUrl);
    return _q;
}

QUrl RedirectToSSOJob::makeRequestUrl(const HomeserverData& hsData, const QString& redirectUrl)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/login/sso/redirect"),
                                   queryToRedirectToSSO(redirectUrl));
}

RedirectToSSOJob::RedirectToSSOJob(const QString& redirectUrl)
    : BaseJob(HttpVerb::Get, u"RedirectToSSOJob"_s,
              makePath("/_matrix/client/v3", "/login/sso/redirect"),
              queryToRedirectToSSO(redirectUrl), {}, false)
{}

auto queryToRedirectToIdP(const QString& redirectUrl)
{
    QUrlQuery _q;
    addParam<>(_q, u"redirectUrl"_s, redirectUrl);
    return _q;
}

QUrl RedirectToIdPJob::makeRequestUrl(const HomeserverData& hsData, const QString& idpId,
                                      const QString& redirectUrl)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/login/sso/redirect/", idpId),
                                   queryToRedirectToIdP(redirectUrl));
}

RedirectToIdPJob::RedirectToIdPJob(const QString& idpId, const QString& redirectUrl)
    : BaseJob(HttpVerb::Get, u"RedirectToIdPJob"_s,
              makePath("/_matrix/client/v3", "/login/sso/redirect/", idpId),
              queryToRedirectToIdP(redirectUrl), {}, false)
{}

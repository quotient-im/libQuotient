/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "sso_login_redirect.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

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

static const auto RedirectToSSOJobName = QStringLiteral("RedirectToSSOJob");

RedirectToSSOJob::RedirectToSSOJob(const QString& redirectUrl)
    : BaseJob(HttpVerb::Get, RedirectToSSOJobName,
              basePath % "/login/sso/redirect",
              queryToRedirectToSSO(redirectUrl), {}, false)
{}

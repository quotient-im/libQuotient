// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "registration_tokens.h"

using namespace Quotient;

auto queryToRegistrationTokenValidity(const QString& token)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("token"), token);
    return _q;
}

QUrl RegistrationTokenValidityJob::makeRequestUrl(QUrl baseUrl, const QString& token)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v1",
                                            "/register/m.login.registration_token/validity"),
                                   queryToRegistrationTokenValidity(token));
}

RegistrationTokenValidityJob::RegistrationTokenValidityJob(const QString& token)
    : BaseJob(HttpVerb::Get, QStringLiteral("RegistrationTokenValidityJob"),
              makePath("/_matrix/client/v1", "/register/m.login.registration_token/validity"),
              queryToRegistrationTokenValidity(token), {}, false)
{
    addExpectedKey("valid");
}

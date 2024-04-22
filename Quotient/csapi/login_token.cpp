// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "login_token.h"

using namespace Quotient;

GenerateLoginTokenJob::GenerateLoginTokenJob(const std::optional<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("GenerateLoginTokenJob"),
              makePath("/_matrix/client/v1", "/login/get_token"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("auth"), auth);
    setRequestData({ _dataJson });
    addExpectedKey("login_token");
    addExpectedKey("expires_in_ms");
}

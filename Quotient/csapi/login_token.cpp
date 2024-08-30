// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "login_token.h"

using namespace Quotient;

GenerateLoginTokenJob::GenerateLoginTokenJob(const std::optional<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, u"GenerateLoginTokenJob"_s,
              makePath("/_matrix/client/v1", "/login/get_token"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "auth"_L1, auth);
    setRequestData({ _dataJson });
    addExpectedKey("login_token");
    addExpectedKey("expires_in_ms");
}

// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "openid.h"

using namespace Quotient;

RequestOpenIdTokenJob::RequestOpenIdTokenJob(const QString& userId, const QJsonObject& dontUse)
    : BaseJob(HttpVerb::Post, u"RequestOpenIdTokenJob"_s,
              makePath("/_matrix/client/v3", "/user/", userId, "/openid/request_token"))
{
    setRequestData({ toJson(dontUse) });
}

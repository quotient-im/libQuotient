/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "openid.h"

using namespace Quotient;

RequestOpenIdTokenJob::RequestOpenIdTokenJob(const QString& userId,
                                             const QJsonObject& body)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestOpenIdTokenJob"),
              makePath("/_matrix/client/r0", "/user/", userId,
                       "/openid/request_token"))
{
    setRequestData(RequestData(toJson(body)));
}

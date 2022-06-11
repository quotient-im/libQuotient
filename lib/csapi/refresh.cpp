/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "refresh.h"

using namespace Quotient;

RefreshJob::RefreshJob(const QString& refreshToken)
    : BaseJob(HttpVerb::Post, QStringLiteral("RefreshJob"),
              makePath("/_matrix/client/v3", "/refresh"), false)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("refresh_token"), refreshToken);
    setRequestData(std::move(_data));
    addExpectedKey("access_token");
}

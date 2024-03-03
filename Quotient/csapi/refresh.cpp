// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "refresh.h"

using namespace Quotient;

RefreshJob::RefreshJob(const QString& refreshToken)
    : BaseJob(HttpVerb::Post, QStringLiteral("RefreshJob"),
              makePath("/_matrix/client/v3", "/refresh"), false)
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("refresh_token"), refreshToken);
    setRequestData({ _dataJson });
    addExpectedKey("access_token");
}

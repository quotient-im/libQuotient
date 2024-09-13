// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "refresh.h"

using namespace Quotient;

RefreshJob::RefreshJob(const QString& refreshToken)
    : BaseJob(HttpVerb::Post, u"RefreshJob"_s, makePath("/_matrix/client/v3", "/refresh"), false)
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "refresh_token"_L1, refreshToken);
    setRequestData({ _dataJson });
    addExpectedKey(u"access_token"_s);
}

// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "search.h"

using namespace Quotient;

auto queryToSearch(const QString& nextBatch)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"next_batch"_s, nextBatch);
    return _q;
}

SearchJob::SearchJob(const Categories& searchCategories, const QString& nextBatch)
    : BaseJob(HttpVerb::Post, u"SearchJob"_s, makePath("/_matrix/client/v3", "/search"),
              queryToSearch(nextBatch))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "search_categories"_L1, searchCategories);
    setRequestData({ _dataJson });
    addExpectedKey("search_categories");
}

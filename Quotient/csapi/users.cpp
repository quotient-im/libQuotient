// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "users.h"

using namespace Quotient;

SearchUserDirectoryJob::SearchUserDirectoryJob(const QString& searchTerm, std::optional<int> limit)
    : BaseJob(HttpVerb::Post, u"SearchUserDirectoryJob"_s,
              makePath("/_matrix/client/v3", "/user_directory/search"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "search_term"_L1, searchTerm);
    addParam<IfNotEmpty>(_dataJson, "limit"_L1, limit);
    setRequestData({ _dataJson });
    addExpectedKey("results");
    addExpectedKey("limited");
}

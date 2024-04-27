// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "users.h"

using namespace Quotient;

SearchUserDirectoryJob::SearchUserDirectoryJob(const QString& searchTerm, std::optional<int> limit)
    : BaseJob(HttpVerb::Post, QStringLiteral("SearchUserDirectoryJob"),
              makePath("/_matrix/client/v3", "/user_directory/search"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("search_term"), searchTerm);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("limit"), limit);
    setRequestData({ _dataJson });
    addExpectedKey("results");
    addExpectedKey("limited");
}

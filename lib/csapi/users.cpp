/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "users.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

// Converters
namespace QMatrixClient
{

template <>
struct JsonObjectConverter<SearchUserDirectoryJob::User>
{
    static void fillFrom(const QJsonObject& jo,
                         SearchUserDirectoryJob::User& result)
    {
        fromJson(jo.value("user_id"_ls), result.userId);
        fromJson(jo.value("display_name"_ls), result.displayName);
        fromJson(jo.value("avatar_url"_ls), result.avatarUrl);
    }
};

} // namespace QMatrixClient

class SearchUserDirectoryJob::Private
{
public:
    QVector<User> results;
    bool limited;
};

static const auto SearchUserDirectoryJobName =
    QStringLiteral("SearchUserDirectoryJob");

SearchUserDirectoryJob::SearchUserDirectoryJob(const QString& searchTerm,
                                               Omittable<int> limit)
    : BaseJob(HttpVerb::Post, SearchUserDirectoryJobName,
              basePath % "/user_directory/search")
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("search_term"), searchTerm);
    addParam<IfNotEmpty>(_data, QStringLiteral("limit"), limit);
    setRequestData(_data);
}

SearchUserDirectoryJob::~SearchUserDirectoryJob() = default;

const QVector<SearchUserDirectoryJob::User>& SearchUserDirectoryJob::results() const
{
    return d->results;
}

bool SearchUserDirectoryJob::limited() const { return d->limited; }

BaseJob::Status SearchUserDirectoryJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("results"_ls))
        return { IncorrectResponse,
                 "The key 'results' not found in the response" };
    fromJson(json.value("results"_ls), d->results);
    if (!json.contains("limited"_ls))
        return { IncorrectResponse,
                 "The key 'limited' not found in the response" };
    fromJson(json.value("limited"_ls), d->limited);

    return Success;
}

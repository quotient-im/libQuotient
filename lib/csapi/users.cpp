/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "users.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    template <> struct FromJson<SearchUserDirectoryJob::User>
    {
        SearchUserDirectoryJob::User operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            SearchUserDirectoryJob::User result;
            result.userId =
                fromJson<QString>(_json.value("user_id"_ls));
            result.displayName =
                fromJson<QString>(_json.value("display_name"_ls));
            result.avatarUrl =
                fromJson<QString>(_json.value("avatar_url"_ls));

            return result;
        }
    };
} // namespace QMatrixClient

class SearchUserDirectoryJob::Private
{
    public:
        QVector<User> results;
        bool limited;
};

static const auto SearchUserDirectoryJobName = QStringLiteral("SearchUserDirectoryJob");

SearchUserDirectoryJob::SearchUserDirectoryJob(const QString& searchTerm, Omittable<int> limit)
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

bool SearchUserDirectoryJob::limited() const
{
    return d->limited;
}

BaseJob::Status SearchUserDirectoryJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("results"_ls))
        return { JsonParseError,
            "The key 'results' not found in the response" };
    d->results = fromJson<QVector<User>>(json.value("results"_ls));
    if (!json.contains("limited"_ls))
        return { JsonParseError,
            "The key 'limited' not found in the response" };
    d->limited = fromJson<bool>(json.value("limited"_ls));
    return Success;
}


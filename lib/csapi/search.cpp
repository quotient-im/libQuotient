/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "search.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const SearchJob::IncludeEventContext& pod)
    {
        QJsonObject _json;
        addParam<IfNotEmpty>(_json, QStringLiteral("before_limit"), pod.beforeLimit);
        addParam<IfNotEmpty>(_json, QStringLiteral("after_limit"), pod.afterLimit);
        addParam<IfNotEmpty>(_json, QStringLiteral("include_profile"), pod.includeProfile);
        return _json;
    }

    QJsonObject toJson(const SearchJob::Group& pod)
    {
        QJsonObject _json;
        addParam<IfNotEmpty>(_json, QStringLiteral("key"), pod.key);
        return _json;
    }

    QJsonObject toJson(const SearchJob::Groupings& pod)
    {
        QJsonObject _json;
        addParam<IfNotEmpty>(_json, QStringLiteral("group_by"), pod.groupBy);
        return _json;
    }

    QJsonObject toJson(const SearchJob::RoomEventsCriteria& pod)
    {
        QJsonObject _json;
        addParam<>(_json, QStringLiteral("search_term"), pod.searchTerm);
        addParam<IfNotEmpty>(_json, QStringLiteral("keys"), pod.keys);
        addParam<IfNotEmpty>(_json, QStringLiteral("filter"), pod.filter);
        addParam<IfNotEmpty>(_json, QStringLiteral("order_by"), pod.orderBy);
        addParam<IfNotEmpty>(_json, QStringLiteral("event_context"), pod.eventContext);
        addParam<IfNotEmpty>(_json, QStringLiteral("include_state"), pod.includeState);
        addParam<IfNotEmpty>(_json, QStringLiteral("groupings"), pod.groupings);
        return _json;
    }

    QJsonObject toJson(const SearchJob::Categories& pod)
    {
        QJsonObject _json;
        addParam<IfNotEmpty>(_json, QStringLiteral("room_events"), pod.roomEvents);
        return _json;
    }

    template <> struct FromJson<SearchJob::UserProfile>
    {
        SearchJob::UserProfile operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            SearchJob::UserProfile result;
            result.displayname =
                fromJson<QString>(_json.value("displayname"_ls));
            result.avatarUrl =
                fromJson<QString>(_json.value("avatar_url"_ls));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::EventContext>
    {
        SearchJob::EventContext operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            SearchJob::EventContext result;
            result.begin =
                fromJson<QString>(_json.value("start"_ls));
            result.end =
                fromJson<QString>(_json.value("end"_ls));
            result.profileInfo =
                fromJson<QHash<QString, SearchJob::UserProfile>>(_json.value("profile_info"_ls));
            result.eventsBefore =
                fromJson<RoomEvents>(_json.value("events_before"_ls));
            result.eventsAfter =
                fromJson<RoomEvents>(_json.value("events_after"_ls));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::Result>
    {
        SearchJob::Result operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            SearchJob::Result result;
            result.rank =
                fromJson<double>(_json.value("rank"_ls));
            result.result =
                fromJson<RoomEventPtr>(_json.value("result"_ls));
            result.context =
                fromJson<SearchJob::EventContext>(_json.value("context"_ls));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::GroupValue>
    {
        SearchJob::GroupValue operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            SearchJob::GroupValue result;
            result.nextBatch =
                fromJson<QString>(_json.value("next_batch"_ls));
            result.order =
                fromJson<int>(_json.value("order"_ls));
            result.results =
                fromJson<QStringList>(_json.value("results"_ls));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::ResultRoomEvents>
    {
        SearchJob::ResultRoomEvents operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            SearchJob::ResultRoomEvents result;
            result.count =
                fromJson<qint64>(_json.value("count"_ls));
            result.highlights =
                fromJson<QStringList>(_json.value("highlights"_ls));
            result.results =
                fromJson<std::vector<SearchJob::Result>>(_json.value("results"_ls));
            result.state =
                fromJson<std::unordered_map<QString, StateEvents>>(_json.value("state"_ls));
            result.groups =
                fromJson<QHash<QString, QHash<QString, SearchJob::GroupValue>>>(_json.value("groups"_ls));
            result.nextBatch =
                fromJson<QString>(_json.value("next_batch"_ls));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::ResultCategories>
    {
        SearchJob::ResultCategories operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            SearchJob::ResultCategories result;
            result.roomEvents =
                fromJson<SearchJob::ResultRoomEvents>(_json.value("room_events"_ls));

            return result;
        }
    };
} // namespace QMatrixClient

class SearchJob::Private
{
    public:
        ResultCategories searchCategories;
};

BaseJob::Query queryToSearch(const QString& nextBatch)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("next_batch"), nextBatch);
    return _q;
}

static const auto SearchJobName = QStringLiteral("SearchJob");

SearchJob::SearchJob(const Categories& searchCategories, const QString& nextBatch)
    : BaseJob(HttpVerb::Post, SearchJobName,
        basePath % "/search",
        queryToSearch(nextBatch))
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("search_categories"), searchCategories);
    setRequestData(_data);
}

SearchJob::~SearchJob() = default;

const SearchJob::ResultCategories& SearchJob::searchCategories() const
{
    return d->searchCategories;
}

BaseJob::Status SearchJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("search_categories"_ls))
        return { JsonParseError,
            "The key 'search_categories' not found in the response" };
    d->searchCategories = fromJson<ResultCategories>(json.value("search_categories"_ls));
    return Success;
}


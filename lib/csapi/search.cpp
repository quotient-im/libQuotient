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
        QJsonObject jo;
        addParam<IfNotEmpty>(jo, QStringLiteral("before_limit"), pod.beforeLimit);
        addParam<IfNotEmpty>(jo, QStringLiteral("after_limit"), pod.afterLimit);
        addParam<IfNotEmpty>(jo, QStringLiteral("include_profile"), pod.includeProfile);
        return jo;
    }

    QJsonObject toJson(const SearchJob::Group& pod)
    {
        QJsonObject jo;
        addParam<IfNotEmpty>(jo, QStringLiteral("key"), pod.key);
        return jo;
    }

    QJsonObject toJson(const SearchJob::Groupings& pod)
    {
        QJsonObject jo;
        addParam<IfNotEmpty>(jo, QStringLiteral("group_by"), pod.groupBy);
        return jo;
    }

    QJsonObject toJson(const SearchJob::RoomEventsCriteria& pod)
    {
        QJsonObject jo;
        addParam<>(jo, QStringLiteral("search_term"), pod.searchTerm);
        addParam<IfNotEmpty>(jo, QStringLiteral("keys"), pod.keys);
        addParam<IfNotEmpty>(jo, QStringLiteral("filter"), pod.filter);
        addParam<IfNotEmpty>(jo, QStringLiteral("order_by"), pod.orderBy);
        addParam<IfNotEmpty>(jo, QStringLiteral("event_context"), pod.eventContext);
        addParam<IfNotEmpty>(jo, QStringLiteral("include_state"), pod.includeState);
        addParam<IfNotEmpty>(jo, QStringLiteral("groupings"), pod.groupings);
        return jo;
    }

    QJsonObject toJson(const SearchJob::Categories& pod)
    {
        QJsonObject jo;
        addParam<IfNotEmpty>(jo, QStringLiteral("room_events"), pod.roomEvents);
        return jo;
    }

    template <> struct FromJsonObject<SearchJob::UserProfile>
    {
        SearchJob::UserProfile operator()(const QJsonObject& jo) const
        {
            SearchJob::UserProfile result;
            result.displayname =
                fromJson<QString>(jo.value("displayname"_ls));
            result.avatarUrl =
                fromJson<QString>(jo.value("avatar_url"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<SearchJob::EventContext>
    {
        SearchJob::EventContext operator()(const QJsonObject& jo) const
        {
            SearchJob::EventContext result;
            result.begin =
                fromJson<QString>(jo.value("start"_ls));
            result.end =
                fromJson<QString>(jo.value("end"_ls));
            result.profileInfo =
                fromJson<QHash<QString, SearchJob::UserProfile>>(jo.value("profile_info"_ls));
            result.eventsBefore =
                fromJson<RoomEvents>(jo.value("events_before"_ls));
            result.eventsAfter =
                fromJson<RoomEvents>(jo.value("events_after"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<SearchJob::Result>
    {
        SearchJob::Result operator()(const QJsonObject& jo) const
        {
            SearchJob::Result result;
            result.rank =
                fromJson<double>(jo.value("rank"_ls));
            result.result =
                fromJson<RoomEventPtr>(jo.value("result"_ls));
            result.context =
                fromJson<SearchJob::EventContext>(jo.value("context"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<SearchJob::GroupValue>
    {
        SearchJob::GroupValue operator()(const QJsonObject& jo) const
        {
            SearchJob::GroupValue result;
            result.nextBatch =
                fromJson<QString>(jo.value("next_batch"_ls));
            result.order =
                fromJson<int>(jo.value("order"_ls));
            result.results =
                fromJson<QStringList>(jo.value("results"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<SearchJob::ResultRoomEvents>
    {
        SearchJob::ResultRoomEvents operator()(const QJsonObject& jo) const
        {
            SearchJob::ResultRoomEvents result;
            result.count =
                fromJson<int>(jo.value("count"_ls));
            result.highlights =
                fromJson<QStringList>(jo.value("highlights"_ls));
            result.results =
                fromJson<std::vector<SearchJob::Result>>(jo.value("results"_ls));
            result.state =
                fromJson<std::unordered_map<QString, StateEvents>>(jo.value("state"_ls));
            result.groups =
                fromJson<QHash<QString, QHash<QString, SearchJob::GroupValue>>>(jo.value("groups"_ls));
            result.nextBatch =
                fromJson<QString>(jo.value("next_batch"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<SearchJob::ResultCategories>
    {
        SearchJob::ResultCategories operator()(const QJsonObject& jo) const
        {
            SearchJob::ResultCategories result;
            result.roomEvents =
                fromJson<SearchJob::ResultRoomEvents>(jo.value("room_events"_ls));

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


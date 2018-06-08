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
        addToJson<IfNotEmpty>(_json, "before_limit", pod.beforeLimit);
        addToJson<IfNotEmpty>(_json, "after_limit", pod.afterLimit);
        addToJson<IfNotEmpty>(_json, "include_profile", pod.includeProfile);
        return _json;
    }

    QJsonObject toJson(const SearchJob::Group& pod)
    {
        QJsonObject _json;
        addToJson<IfNotEmpty>(_json, "key", pod.key);
        return _json;
    }

    QJsonObject toJson(const SearchJob::Groupings& pod)
    {
        QJsonObject _json;
        addToJson<IfNotEmpty>(_json, "group_by", pod.groupBy);
        return _json;
    }

    QJsonObject toJson(const SearchJob::RoomEventsCriteria& pod)
    {
        QJsonObject _json;
        addToJson<>(_json, "search_term", pod.searchTerm);
        addToJson<IfNotEmpty>(_json, "keys", pod.keys);
        addToJson<IfNotEmpty>(_json, "filter", pod.filter);
        addToJson<IfNotEmpty>(_json, "order_by", pod.orderBy);
        addToJson<IfNotEmpty>(_json, "event_context", pod.eventContext);
        addToJson<IfNotEmpty>(_json, "include_state", pod.includeState);
        addToJson<IfNotEmpty>(_json, "groupings", pod.groupings);
        return _json;
    }

    QJsonObject toJson(const SearchJob::Categories& pod)
    {
        QJsonObject _json;
        addToJson<IfNotEmpty>(_json, "room_events", pod.roomEvents);
        return _json;
    }

    template <> struct FromJson<SearchJob::UserProfile>
    {
        SearchJob::UserProfile operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            SearchJob::UserProfile result;
            result.displayname =
                fromJson<QString>(_json.value("displayname"));
            result.avatarUrl =
                fromJson<QString>(_json.value("avatar_url"));

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
                fromJson<QString>(_json.value("start"));
            result.end =
                fromJson<QString>(_json.value("end"));
            result.profileInfo =
                fromJson<QHash<QString, SearchJob::UserProfile>>(_json.value("profile_info"));
            result.eventsBefore =
                fromJson<RoomEvents>(_json.value("events_before"));
            result.eventsAfter =
                fromJson<RoomEvents>(_json.value("events_after"));

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
                fromJson<double>(_json.value("rank"));
            result.result =
                fromJson<RoomEventPtr>(_json.value("result"));
            result.context =
                fromJson<SearchJob::EventContext>(_json.value("context"));

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
                fromJson<QString>(_json.value("next_batch"));
            result.order =
                fromJson<int>(_json.value("order"));
            result.results =
                fromJson<QStringList>(_json.value("results"));

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
                fromJson<qint64>(_json.value("count"));
            result.results =
                fromJson<std::vector<SearchJob::Result>>(_json.value("results"));
            result.state =
                fromJson<std::unordered_map<QString, StateEvents>>(_json.value("state"));
            result.groups =
                fromJson<QHash<QString, QHash<QString, SearchJob::GroupValue>>>(_json.value("groups"));
            result.nextBatch =
                fromJson<QString>(_json.value("next_batch"));

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
                fromJson<SearchJob::ResultRoomEvents>(_json.value("room_events"));

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
    addToQuery<IfNotEmpty>(_q, "next_batch", nextBatch);
    return _q;
}

SearchJob::SearchJob(const Categories& searchCategories, const QString& nextBatch)
    : BaseJob(HttpVerb::Post, "SearchJob",
        basePath % "/search",
        queryToSearch(nextBatch))
    , d(new Private)
{
    QJsonObject _data;
    addToJson<>(_data, "search_categories", searchCategories);
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
    if (!json.contains("search_categories"))
        return { JsonParseError,
            "The key 'search_categories' not found in the response" };
    d->searchCategories = fromJson<ResultCategories>(json.value("search_categories"));
    return Success;
}


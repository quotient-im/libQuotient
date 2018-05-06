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
        QJsonObject o;
        o.insert("before_limit", toJson(pod.beforeLimit));
        o.insert("after_limit", toJson(pod.afterLimit));
        o.insert("include_profile", toJson(pod.includeProfile));

        return o;
    }

    QJsonObject toJson(const SearchJob::Group& pod)
    {
        QJsonObject o;
        o.insert("key", toJson(pod.key));

        return o;
    }

    QJsonObject toJson(const SearchJob::Groupings& pod)
    {
        QJsonObject o;
        o.insert("group_by", toJson(pod.groupBy));

        return o;
    }

    QJsonObject toJson(const SearchJob::RoomEventsCriteria& pod)
    {
        QJsonObject o;
        o.insert("search_term", toJson(pod.searchTerm));
        o.insert("keys", toJson(pod.keys));
        o.insert("filter", toJson(pod.filter));
        o.insert("order_by", toJson(pod.orderBy));
        o.insert("event_context", toJson(pod.eventContext));
        o.insert("include_state", toJson(pod.includeState));
        o.insert("groupings", toJson(pod.groupings));

        return o;
    }

    QJsonObject toJson(const SearchJob::Categories& pod)
    {
        QJsonObject o;
        o.insert("room_events", toJson(pod.roomEvents));

        return o;
    }

    template <> struct FromJson<SearchJob::UserProfile>
    {
        SearchJob::UserProfile operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            SearchJob::UserProfile result;
            result.displayname =
                fromJson<QString>(o.value("displayname"));
            result.avatarUrl =
                fromJson<QString>(o.value("avatar_url"));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::EventContext>
    {
        SearchJob::EventContext operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            SearchJob::EventContext result;
            result.begin =
                fromJson<QString>(o.value("start"));
            result.end =
                fromJson<QString>(o.value("end"));
            result.profileInfo =
                fromJson<QHash<QString, SearchJob::UserProfile>>(o.value("profile_info"));
            result.eventsBefore =
                fromJson<RoomEvents>(o.value("events_before"));
            result.eventsAfter =
                fromJson<RoomEvents>(o.value("events_after"));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::Result>
    {
        SearchJob::Result operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            SearchJob::Result result;
            result.rank =
                fromJson<double>(o.value("rank"));
            result.result =
                fromJson<RoomEventPtr>(o.value("result"));
            result.context =
                fromJson<SearchJob::EventContext>(o.value("context"));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::GroupValue>
    {
        SearchJob::GroupValue operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            SearchJob::GroupValue result;
            result.nextBatch =
                fromJson<QString>(o.value("next_batch"));
            result.order =
                fromJson<int>(o.value("order"));
            result.results =
                fromJson<QStringList>(o.value("results"));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::ResultRoomEvents>
    {
        SearchJob::ResultRoomEvents operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            SearchJob::ResultRoomEvents result;
            result.count =
                fromJson<qint64>(o.value("count"));
            result.results =
                fromJson<std::vector<SearchJob::Result>>(o.value("results"));
            result.state =
                fromJson<std::unordered_map<QString, StateEvents>>(o.value("state"));
            result.groups =
                fromJson<QHash<QString, QHash<QString, SearchJob::GroupValue>>>(o.value("groups"));
            result.nextBatch =
                fromJson<QString>(o.value("next_batch"));

            return result;
        }
    };

    template <> struct FromJson<SearchJob::ResultCategories>
    {
        SearchJob::ResultCategories operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            SearchJob::ResultCategories result;
            result.roomEvents =
                fromJson<SearchJob::ResultRoomEvents>(o.value("room_events"));

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
    if (!nextBatch.isEmpty())
        _q.addQueryItem("next_batch", nextBatch);
    return _q;
}

SearchJob::SearchJob(const Categories& searchCategories, const QString& nextBatch)
    : BaseJob(HttpVerb::Post, "SearchJob",
        basePath % "/search",
        queryToSearch(nextBatch))
    , d(new Private)
{
    QJsonObject _data;
    _data.insert("search_categories", toJson(searchCategories));
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


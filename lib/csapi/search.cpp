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

    template <> struct JsonObjectConverter<SearchJob::IncludeEventContext>
    {
        static void dumpTo(QJsonObject& jo, const SearchJob::IncludeEventContext& pod)
        {
            addParam<IfNotEmpty>(jo, QStringLiteral("before_limit"), pod.beforeLimit);
            addParam<IfNotEmpty>(jo, QStringLiteral("after_limit"), pod.afterLimit);
            addParam<IfNotEmpty>(jo, QStringLiteral("include_profile"), pod.includeProfile);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::Group>
    {
        static void dumpTo(QJsonObject& jo, const SearchJob::Group& pod)
        {
            addParam<IfNotEmpty>(jo, QStringLiteral("key"), pod.key);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::Groupings>
    {
        static void dumpTo(QJsonObject& jo, const SearchJob::Groupings& pod)
        {
            addParam<IfNotEmpty>(jo, QStringLiteral("group_by"), pod.groupBy);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::RoomEventsCriteria>
    {
        static void dumpTo(QJsonObject& jo, const SearchJob::RoomEventsCriteria& pod)
        {
            addParam<>(jo, QStringLiteral("search_term"), pod.searchTerm);
            addParam<IfNotEmpty>(jo, QStringLiteral("keys"), pod.keys);
            addParam<IfNotEmpty>(jo, QStringLiteral("filter"), pod.filter);
            addParam<IfNotEmpty>(jo, QStringLiteral("order_by"), pod.orderBy);
            addParam<IfNotEmpty>(jo, QStringLiteral("event_context"), pod.eventContext);
            addParam<IfNotEmpty>(jo, QStringLiteral("include_state"), pod.includeState);
            addParam<IfNotEmpty>(jo, QStringLiteral("groupings"), pod.groupings);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::Categories>
    {
        static void dumpTo(QJsonObject& jo, const SearchJob::Categories& pod)
        {
            addParam<IfNotEmpty>(jo, QStringLiteral("room_events"), pod.roomEvents);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::UserProfile>
    {
        static void fillFrom(const QJsonObject& jo, SearchJob::UserProfile& result)
        {
            fromJson(jo.value("displayname"_ls), result.displayname);
            fromJson(jo.value("avatar_url"_ls), result.avatarUrl);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::EventContext>
    {
        static void fillFrom(const QJsonObject& jo, SearchJob::EventContext& result)
        {
            fromJson(jo.value("start"_ls), result.begin);
            fromJson(jo.value("end"_ls), result.end);
            fromJson(jo.value("profile_info"_ls), result.profileInfo);
            fromJson(jo.value("events_before"_ls), result.eventsBefore);
            fromJson(jo.value("events_after"_ls), result.eventsAfter);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::Result>
    {
        static void fillFrom(const QJsonObject& jo, SearchJob::Result& result)
        {
            fromJson(jo.value("rank"_ls), result.rank);
            fromJson(jo.value("result"_ls), result.result);
            fromJson(jo.value("context"_ls), result.context);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::GroupValue>
    {
        static void fillFrom(const QJsonObject& jo, SearchJob::GroupValue& result)
        {
            fromJson(jo.value("next_batch"_ls), result.nextBatch);
            fromJson(jo.value("order"_ls), result.order);
            fromJson(jo.value("results"_ls), result.results);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::ResultRoomEvents>
    {
        static void fillFrom(const QJsonObject& jo, SearchJob::ResultRoomEvents& result)
        {
            fromJson(jo.value("count"_ls), result.count);
            fromJson(jo.value("highlights"_ls), result.highlights);
            fromJson(jo.value("results"_ls), result.results);
            fromJson(jo.value("state"_ls), result.state);
            fromJson(jo.value("groups"_ls), result.groups);
            fromJson(jo.value("next_batch"_ls), result.nextBatch);
        }
    };

    template <> struct JsonObjectConverter<SearchJob::ResultCategories>
    {
        static void fillFrom(const QJsonObject& jo, SearchJob::ResultCategories& result)
        {
            fromJson(jo.value("room_events"_ls), result.roomEvents);
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
    fromJson(json.value("search_categories"_ls), d->searchCategories);
    return Success;
}


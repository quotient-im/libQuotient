// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/room_event_filter.h>

#include <Quotient/events/roomevent.h>
#include <Quotient/events/stateevent.h>
#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Perform a server-side search.
//!
//! Performs a full text search across different categories.
class QUOTIENT_API SearchJob : public BaseJob {
public:
    // Inner data structures

    //! Configures whether any context for the events
    //! returned are included in the response.
    struct QUOTIENT_API IncludeEventContext {
        //! How many events before the result are
        //! returned. By default, this is `5`.
        std::optional<int> beforeLimit{};

        //! How many events after the result are
        //! returned. By default, this is `5`.
        std::optional<int> afterLimit{};

        //! Requests that the server returns the
        //! historic profile information for the users
        //! that sent the events that were returned.
        //! By default, this is `false`.
        std::optional<bool> includeProfile{};
    };

    //! Configuration for group.
    struct QUOTIENT_API Group {
        //! Key that defines the group.
        QString key{};
    };

    //! Requests that the server partitions the result set
    //! based on the provided list of keys.
    struct QUOTIENT_API Groupings {
        //! List of groups to request.
        QVector<Group> groupBy{};
    };

    //! Mapping of category name to search criteria.
    struct QUOTIENT_API RoomEventsCriteria {
        //! The string to search events for
        QString searchTerm;

        //! The keys to search. Defaults to all.
        QStringList keys{};

        //! This takes a [filter](/client-server-api/#filtering).
        RoomEventFilter filter{};

        //! The order in which to search for results.
        //! By default, this is `"rank"`.
        QString orderBy{};

        //! Configures whether any context for the events
        //! returned are included in the response.
        std::optional<IncludeEventContext> eventContext{};

        //! Requests the server return the current state for
        //! each room returned.
        std::optional<bool> includeState{};

        //! Requests that the server partitions the result set
        //! based on the provided list of keys.
        std::optional<Groupings> groupings{};
    };

    //! Describes which categories to search in and their criteria.
    struct QUOTIENT_API Categories {
        //! Mapping of category name to search criteria.
        std::optional<RoomEventsCriteria> roomEvents{};
    };

    struct QUOTIENT_API UserProfile {
        QString displayname{};

        QUrl avatarUrl{};
    };

    //! Context for result, if requested.
    struct QUOTIENT_API EventContext {
        //! Pagination token for the start of the chunk
        QString begin{};

        //! Pagination token for the end of the chunk
        QString end{};

        //! The historic profile information of the
        //! users that sent the events returned.
        //!
        //! The key is the user ID for which
        //! the profile belongs to.
        QHash<UserId, UserProfile> profileInfo{};

        //! Events just before the result.
        RoomEvents eventsBefore{};

        //! Events just after the result.
        RoomEvents eventsAfter{};
    };

    //! The result object.
    struct QUOTIENT_API Result {
        //! A number that describes how closely this result matches the search. Higher is closer.
        std::optional<double> rank{};

        //! The event that matched.
        RoomEventPtr result{};

        //! Context for result, if requested.
        std::optional<EventContext> context{};
    };

    //! The results for a particular group value.
    struct QUOTIENT_API GroupValue {
        //! Token that can be used to get the next batch
        //! of results in the group, by passing as the
        //! `next_batch` parameter to the next call. If
        //! this field is absent, there are no more
        //! results in this group.
        QString nextBatch{};

        //! Key that can be used to order different
        //! groups.
        std::optional<int> order{};

        //! Which results are in this group.
        QStringList results{};
    };

    //! Mapping of category name to search criteria.
    struct QUOTIENT_API ResultRoomEvents {
        //! An approximate count of the total number of results found.
        std::optional<int> count{};

        //! List of words which should be highlighted, useful for stemming which may change the
        //! query terms.
        QStringList highlights{};

        //! List of results in the requested order.
        std::vector<Result> results{};

        //! The current state for every room in the results.
        //! This is included if the request had the
        //! `include_state` key set with a value of `true`.
        //!
        //! The key is the room ID for which the `State
        //! Event` array belongs to.
        std::unordered_map<RoomId, StateEvents> state{};

        //! Any groups that were requested.
        //!
        //! The outer `string` key is the group key requested (eg: `room_id`
        //! or `sender`). The inner `string` key is the grouped value (eg:
        //! a room's ID or a user's ID).
        QHash<QString, QHash<QString, GroupValue>> groups{};

        //! Token that can be used to get the next batch of
        //! results, by passing as the `next_batch` parameter to
        //! the next call. If this field is absent, there are no
        //! more results.
        QString nextBatch{};
    };

    //! Describes which categories to search in and their criteria.
    struct QUOTIENT_API ResultCategories {
        //! Mapping of category name to search criteria.
        std::optional<ResultRoomEvents> roomEvents{};
    };

    // Construction/destruction

    //! \param searchCategories
    //!   Describes which categories to search in and their criteria.
    //!
    //! \param nextBatch
    //!   The point to return events from. If given, this should be a
    //!   `next_batch` result from a previous call to this endpoint.
    explicit SearchJob(const Categories& searchCategories, const QString& nextBatch = {});

    // Result properties

    //! Describes which categories to search in and their criteria.
    ResultCategories searchCategories() const
    {
        return loadFromJson<ResultCategories>("search_categories"_ls);
    }
};

inline auto collectResponse(const SearchJob* job) { return job->searchCategories(); }

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::IncludeEventContext> {
    static void dumpTo(QJsonObject& jo, const SearchJob::IncludeEventContext& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("before_limit"), pod.beforeLimit);
        addParam<IfNotEmpty>(jo, QStringLiteral("after_limit"), pod.afterLimit);
        addParam<IfNotEmpty>(jo, QStringLiteral("include_profile"), pod.includeProfile);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::Group> {
    static void dumpTo(QJsonObject& jo, const SearchJob::Group& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("key"), pod.key);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::Groupings> {
    static void dumpTo(QJsonObject& jo, const SearchJob::Groupings& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("group_by"), pod.groupBy);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::RoomEventsCriteria> {
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

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::Categories> {
    static void dumpTo(QJsonObject& jo, const SearchJob::Categories& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("room_events"), pod.roomEvents);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::UserProfile> {
    static void fillFrom(const QJsonObject& jo, SearchJob::UserProfile& result)
    {
        fillFromJson(jo.value("displayname"_ls), result.displayname);
        fillFromJson(jo.value("avatar_url"_ls), result.avatarUrl);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::EventContext> {
    static void fillFrom(const QJsonObject& jo, SearchJob::EventContext& result)
    {
        fillFromJson(jo.value("start"_ls), result.begin);
        fillFromJson(jo.value("end"_ls), result.end);
        fillFromJson(jo.value("profile_info"_ls), result.profileInfo);
        fillFromJson(jo.value("events_before"_ls), result.eventsBefore);
        fillFromJson(jo.value("events_after"_ls), result.eventsAfter);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::Result> {
    static void fillFrom(const QJsonObject& jo, SearchJob::Result& result)
    {
        fillFromJson(jo.value("rank"_ls), result.rank);
        fillFromJson(jo.value("result"_ls), result.result);
        fillFromJson(jo.value("context"_ls), result.context);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::GroupValue> {
    static void fillFrom(const QJsonObject& jo, SearchJob::GroupValue& result)
    {
        fillFromJson(jo.value("next_batch"_ls), result.nextBatch);
        fillFromJson(jo.value("order"_ls), result.order);
        fillFromJson(jo.value("results"_ls), result.results);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::ResultRoomEvents> {
    static void fillFrom(const QJsonObject& jo, SearchJob::ResultRoomEvents& result)
    {
        fillFromJson(jo.value("count"_ls), result.count);
        fillFromJson(jo.value("highlights"_ls), result.highlights);
        fillFromJson(jo.value("results"_ls), result.results);
        fillFromJson(jo.value("state"_ls), result.state);
        fillFromJson(jo.value("groups"_ls), result.groups);
        fillFromJson(jo.value("next_batch"_ls), result.nextBatch);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<SearchJob::ResultCategories> {
    static void fillFrom(const QJsonObject& jo, SearchJob::ResultCategories& result)
    {
        fillFromJson(jo.value("room_events"_ls), result.roomEvents);
    }
};

} // namespace Quotient

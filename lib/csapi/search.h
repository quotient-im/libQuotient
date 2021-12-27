/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "csapi/definitions/room_event_filter.h"

#include "events/eventloader.h"
#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Perform a server-side search.
 *
 * Performs a full text search across different categories.
 */
class SearchJob : public BaseJob {
public:
    // Inner data structures

    /// Configures whether any context for the events
    /// returned are included in the response.
    struct IncludeEventContext {
        /// How many events before the result are
        /// returned. By default, this is `5`.
        Omittable<int> beforeLimit;
        /// How many events after the result are
        /// returned. By default, this is `5`.
        Omittable<int> afterLimit;
        /// Requests that the server returns the
        /// historic profile information for the users
        /// that sent the events that were returned.
        /// By default, this is `false`.
        Omittable<bool> includeProfile;
    };

    /// Configuration for group.
    struct Group {
        /// Key that defines the group.
        QString key;
    };

    /// Requests that the server partitions the result set
    /// based on the provided list of keys.
    struct Groupings {
        /// List of groups to request.
        QVector<Group> groupBy;
    };

    /// Mapping of category name to search criteria.
    struct RoomEventsCriteria {
        /// The string to search events for
        QString searchTerm;
        /// The keys to search. Defaults to all.
        QStringList keys;
        /// This takes a [filter](/client-server-api/#filtering).
        RoomEventFilter filter;
        /// The order in which to search for results.
        /// By default, this is `"rank"`.
        QString orderBy;
        /// Configures whether any context for the events
        /// returned are included in the response.
        Omittable<IncludeEventContext> eventContext;
        /// Requests the server return the current state for
        /// each room returned.
        Omittable<bool> includeState;
        /// Requests that the server partitions the result set
        /// based on the provided list of keys.
        Omittable<Groupings> groupings;
    };

    /// Describes which categories to search in and their criteria.
    struct Categories {
        /// Mapping of category name to search criteria.
        Omittable<RoomEventsCriteria> roomEvents;
    };

    /// Performs a full text search across different categories.
    struct UserProfile {
        /// Performs a full text search across different categories.
        QString displayname;
        /// Performs a full text search across different categories.
        QUrl avatarUrl;
    };

    /// Context for result, if requested.
    struct EventContext {
        /// Pagination token for the start of the chunk
        QString begin;
        /// Pagination token for the end of the chunk
        QString end;
        /// The historic profile information of the
        /// users that sent the events returned.
        ///
        /// The `string` key is the user ID for which
        /// the profile belongs to.
        QHash<QString, UserProfile> profileInfo;
        /// Events just before the result.
        RoomEvents eventsBefore;
        /// Events just after the result.
        RoomEvents eventsAfter;
    };

    /// The result object.
    struct Result {
        /// A number that describes how closely this result matches the search.
        /// Higher is closer.
        Omittable<double> rank;
        /// The event that matched.
        RoomEventPtr result;
        /// Context for result, if requested.
        Omittable<EventContext> context;
    };

    /// The results for a particular group value.
    struct GroupValue {
        /// Token that can be used to get the next batch
        /// of results in the group, by passing as the
        /// `next_batch` parameter to the next call. If
        /// this field is absent, there are no more
        /// results in this group.
        QString nextBatch;
        /// Key that can be used to order different
        /// groups.
        Omittable<int> order;
        /// Which results are in this group.
        QStringList results;
    };

    /// Mapping of category name to search criteria.
    struct ResultRoomEvents {
        /// An approximate count of the total number of results found.
        Omittable<int> count;
        /// List of words which should be highlighted, useful for stemming which
        /// may change the query terms.
        QStringList highlights;
        /// List of results in the requested order.
        std::vector<Result> results;
        /// The current state for every room in the results.
        /// This is included if the request had the
        /// `include_state` key set with a value of `true`.
        ///
        /// The `string` key is the room ID for which the `State
        /// Event` array belongs to.
        UnorderedMap<QString, StateEvents> state;
        /// Any groups that were requested.
        ///
        /// The outer `string` key is the group key requested (eg: `room_id`
        /// or `sender`). The inner `string` key is the grouped value (eg:
        /// a room's ID or a user's ID).
        QHash<QString, QHash<QString, GroupValue>> groups;
        /// Token that can be used to get the next batch of
        /// results, by passing as the `next_batch` parameter to
        /// the next call. If this field is absent, there are no
        /// more results.
        QString nextBatch;
    };

    /// Describes which categories to search in and their criteria.
    struct ResultCategories {
        /// Mapping of category name to search criteria.
        Omittable<ResultRoomEvents> roomEvents;
    };

    // Construction/destruction

    /*! \brief Perform a server-side search.
     *
     * \param searchCategories
     *   Describes which categories to search in and their criteria.
     *
     * \param nextBatch
     *   The point to return events from. If given, this should be a
     *   `next_batch` result from a previous call to this endpoint.
     */
    explicit SearchJob(const Categories& searchCategories,
                       const QString& nextBatch = {});

    // Result properties

    /// Describes which categories to search in and their criteria.
    ResultCategories searchCategories() const
    {
        return loadFromJson<ResultCategories>("search_categories"_ls);
    }
};

template <>
struct JsonObjectConverter<SearchJob::IncludeEventContext> {
    static void dumpTo(QJsonObject& jo,
                       const SearchJob::IncludeEventContext& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("before_limit"),
                             pod.beforeLimit);
        addParam<IfNotEmpty>(jo, QStringLiteral("after_limit"), pod.afterLimit);
        addParam<IfNotEmpty>(jo, QStringLiteral("include_profile"),
                             pod.includeProfile);
    }
};

template <>
struct JsonObjectConverter<SearchJob::Group> {
    static void dumpTo(QJsonObject& jo, const SearchJob::Group& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("key"), pod.key);
    }
};

template <>
struct JsonObjectConverter<SearchJob::Groupings> {
    static void dumpTo(QJsonObject& jo, const SearchJob::Groupings& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("group_by"), pod.groupBy);
    }
};

template <>
struct JsonObjectConverter<SearchJob::RoomEventsCriteria> {
    static void dumpTo(QJsonObject& jo, const SearchJob::RoomEventsCriteria& pod)
    {
        addParam<>(jo, QStringLiteral("search_term"), pod.searchTerm);
        addParam<IfNotEmpty>(jo, QStringLiteral("keys"), pod.keys);
        addParam<IfNotEmpty>(jo, QStringLiteral("filter"), pod.filter);
        addParam<IfNotEmpty>(jo, QStringLiteral("order_by"), pod.orderBy);
        addParam<IfNotEmpty>(jo, QStringLiteral("event_context"),
                             pod.eventContext);
        addParam<IfNotEmpty>(jo, QStringLiteral("include_state"),
                             pod.includeState);
        addParam<IfNotEmpty>(jo, QStringLiteral("groupings"), pod.groupings);
    }
};

template <>
struct JsonObjectConverter<SearchJob::Categories> {
    static void dumpTo(QJsonObject& jo, const SearchJob::Categories& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("room_events"), pod.roomEvents);
    }
};

template <>
struct JsonObjectConverter<SearchJob::UserProfile> {
    static void fillFrom(const QJsonObject& jo, SearchJob::UserProfile& result)
    {
        fromJson(jo.value("displayname"_ls), result.displayname);
        fromJson(jo.value("avatar_url"_ls), result.avatarUrl);
    }
};

template <>
struct JsonObjectConverter<SearchJob::EventContext> {
    static void fillFrom(const QJsonObject& jo, SearchJob::EventContext& result)
    {
        fromJson(jo.value("start"_ls), result.begin);
        fromJson(jo.value("end"_ls), result.end);
        fromJson(jo.value("profile_info"_ls), result.profileInfo);
        fromJson(jo.value("events_before"_ls), result.eventsBefore);
        fromJson(jo.value("events_after"_ls), result.eventsAfter);
    }
};

template <>
struct JsonObjectConverter<SearchJob::Result> {
    static void fillFrom(const QJsonObject& jo, SearchJob::Result& result)
    {
        fromJson(jo.value("rank"_ls), result.rank);
        fromJson(jo.value("result"_ls), result.result);
        fromJson(jo.value("context"_ls), result.context);
    }
};

template <>
struct JsonObjectConverter<SearchJob::GroupValue> {
    static void fillFrom(const QJsonObject& jo, SearchJob::GroupValue& result)
    {
        fromJson(jo.value("next_batch"_ls), result.nextBatch);
        fromJson(jo.value("order"_ls), result.order);
        fromJson(jo.value("results"_ls), result.results);
    }
};

template <>
struct JsonObjectConverter<SearchJob::ResultRoomEvents> {
    static void fillFrom(const QJsonObject& jo,
                         SearchJob::ResultRoomEvents& result)
    {
        fromJson(jo.value("count"_ls), result.count);
        fromJson(jo.value("highlights"_ls), result.highlights);
        fromJson(jo.value("results"_ls), result.results);
        fromJson(jo.value("state"_ls), result.state);
        fromJson(jo.value("groups"_ls), result.groups);
        fromJson(jo.value("next_batch"_ls), result.nextBatch);
    }
};

template <>
struct JsonObjectConverter<SearchJob::ResultCategories> {
    static void fillFrom(const QJsonObject& jo,
                         SearchJob::ResultCategories& result)
    {
        fromJson(jo.value("room_events"_ls), result.roomEvents);
    }
};

} // namespace Quotient

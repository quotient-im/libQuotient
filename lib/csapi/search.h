/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>
#include "converters.h"
#include <QtCore/QVector>
#include <unordered_map>
#include <QtCore/QHash>
#include "events/eventloader.h"

namespace QMatrixClient
{
    // Operations

    /// Perform a server-side search.
    /// 
    /// Performs a full text search across different categories.
    class SearchJob : public BaseJob
    {
        public:
            // Inner data structures

            /// Configures whether any context for the events
            /// returned are included in the response.
            struct IncludeEventContext
            {
                /// How many events before the result are
                /// returned. By default, this is ``5``.
                Omittable<int> beforeLimit;
                /// How many events after the result are
                /// returned. By default, this is ``5``.
                Omittable<int> afterLimit;
                /// Requests that the server returns the
                /// historic profile information for the users
                /// that sent the events that were returned.
                /// By default, this is ``false``.
                bool includeProfile;
            };

            /// Configuration for group.
            struct Group
            {
                /// Key that defines the group.
                QString key;
            };

            /// Requests that the server partitions the result set
            /// based on the provided list of keys.
            struct Groupings
            {
                /// List of groups to request.
                QVector<Group> groupBy;
            };

            /// Mapping of category name to search criteria.
            struct RoomEventsCriteria
            {
                /// The string to search events for
                QString searchTerm;
                /// The keys to search. Defaults to all.
                QStringList keys;
                /// This takes a `filter`_.
                QJsonObject filter;
                /// The order in which to search for results.
                /// By default, this is ``"rank"``.
                QString orderBy;
                /// Configures whether any context for the events
                /// returned are included in the response.
                Omittable<IncludeEventContext> eventContext;
                /// Requests the server return the current state for
                /// each room returned.
                bool includeState;
                /// Requests that the server partitions the result set
                /// based on the provided list of keys.
                Omittable<Groupings> groupings;
            };

            /// Describes which categories to search in and their criteria.
            struct Categories
            {
                /// Mapping of category name to search criteria.
                Omittable<RoomEventsCriteria> roomEvents;
            };

            /// Performs a full text search across different categories.
            struct UserProfile
            {
                /// Performs a full text search across different categories.
                QString displayname;
                /// Performs a full text search across different categories.
                QString avatarUrl;
            };

            /// Context for result, if requested.
            struct EventContext
            {
                /// Pagination token for the start of the chunk
                QString begin;
                /// Pagination token for the end of the chunk
                QString end;
                /// The historic profile information of the
                /// users that sent the events returned.
                /// 
                /// The ``string`` key is the user ID for which
                /// the profile belongs to.
                QHash<QString, UserProfile> profileInfo;
                /// Events just before the result.
                RoomEvents eventsBefore;
                /// Events just after the result.
                RoomEvents eventsAfter;
            };

            /// The result object.
            struct Result
            {
                /// A number that describes how closely this result matches the search. Higher is closer.
                Omittable<double> rank;
                /// The event that matched.
                RoomEventPtr result;
                /// Context for result, if requested.
                Omittable<EventContext> context;
            };

            /// The results for a particular group value.
            struct GroupValue
            {
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
            struct ResultRoomEvents
            {
                /// An approximate count of the total number of results found.
                Omittable<qint64> count;
                /// List of words which should be highlighted, useful for stemming which may change the query terms.
                QStringList highlights;
                /// List of results in the requested order.
                std::vector<Result> results;
                /// The current state for every room in the results.
                /// This is included if the request had the
                /// ``include_state`` key set with a value of ``true``.
                /// 
                /// The ``string`` key is the room ID for which the ``State
                /// Event`` array belongs to.
                std::unordered_map<QString, StateEvents> state;
                /// Any groups that were requested.
                /// 
                /// The outer ``string`` key is the group key requested (eg: ``room_id``
                /// or ``sender``). The inner ``string`` key is the grouped value (eg: 
                /// a room's ID or a user's ID).
                QHash<QString, QHash<QString, GroupValue>> groups;
                /// Token that can be used to get the next batch of
                /// results, by passing as the `next_batch` parameter to
                /// the next call. If this field is absent, there are no
                /// more results.
                QString nextBatch;
            };

            /// Describes which categories to search in and their criteria.
            struct ResultCategories
            {
                /// Mapping of category name to search criteria.
                Omittable<ResultRoomEvents> roomEvents;
            };

            // Construction/destruction

            /*! Perform a server-side search.
             * \param searchCategories 
             *   Describes which categories to search in and their criteria.
             * \param nextBatch 
             *   The point to return events from. If given, this should be a
             *   `next_batch` result from a previous call to this endpoint.
             */
            explicit SearchJob(const Categories& searchCategories, const QString& nextBatch = {});
            ~SearchJob() override;

            // Result properties

            /// Describes which categories to search in and their criteria.
            const ResultCategories& searchCategories() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

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

    class SearchJob : public BaseJob
    {
        public:
            // Inner data structures

            struct IncludeEventContext
            {
                Omittable<int> beforeLimit;
                Omittable<int> afterLimit;
                bool includeProfile;
            };

            struct Group
            {
                QString key;
            };

            struct Groupings
            {
                QVector<Group> groupBy;
            };

            struct RoomEventsCriteria
            {
                QString searchTerm;
                QStringList keys;
                QJsonObject filter;
                QString orderBy;
                Omittable<IncludeEventContext> eventContext;
                bool includeState;
                Omittable<Groupings> groupings;
            };

            struct Categories
            {
                Omittable<RoomEventsCriteria> roomEvents;
            };

            struct UserProfile
            {
                QString displayname;
                QString avatarUrl;
            };

            struct EventContext
            {
                QString begin;
                QString end;
                QHash<QString, UserProfile> profileInfo;
                RoomEvents eventsBefore;
                RoomEvents eventsAfter;
            };

            struct Result
            {
                Omittable<double> rank;
                RoomEventPtr result;
                Omittable<EventContext> context;
            };

            struct GroupValue
            {
                QString nextBatch;
                Omittable<int> order;
                QStringList results;
            };

            struct ResultRoomEvents
            {
                Omittable<qint64> count;
                std::vector<Result> results;
                std::unordered_map<QString, StateEvents> state;
                QHash<QString, QHash<QString, GroupValue>> groups;
                QString nextBatch;
            };

            struct ResultCategories
            {
                Omittable<ResultRoomEvents> roomEvents;
            };

            // Construction/destruction

            explicit SearchJob(const Categories& searchCategories, const QString& nextBatch = {});
            ~SearchJob() override;

            // Result properties

            const ResultCategories& searchCategories() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

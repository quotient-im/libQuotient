/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <unordered_map>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include "events/event.h"
#include <QtCore/QStringList>
#include <QtCore/QVector>

namespace QMatrixClient
{
    // Operations

    class SearchJob : public BaseJob
    {
        public:
            // Inner data structures

            struct IncludeEventContext
            {
                int beforeLimit;
                int afterLimit;
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
                IncludeEventContext eventContext;
                bool includeState;
                Groupings groupings;
            };

            struct Categories
            {
                RoomEventsCriteria roomEvents;
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
                double rank;
                RoomEventPtr result;
                EventContext context;
            };

            struct GroupValue
            {
                QString nextBatch;
                int order;
                QStringList results;
            };

            struct ResultRoomEvents
            {
                qint64 count;
                std::vector<Result> results;
                std::unordered_map<QString, StateEvents> state;
                QHash<QString, QHash<QString, GroupValue>> groups;
                QString nextBatch;
            };

            struct ResultCategories
            {
                ResultRoomEvents roomEvents;
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

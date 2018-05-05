/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QVector>
#include <QtCore/QStringList>

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class GetRoomVisibilityOnDirectoryJob : public BaseJob
    {
        public:
            explicit GetRoomVisibilityOnDirectoryJob(const QString& roomId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetRoomVisibilityOnDirectoryJob. This function can be used when
             * a URL for GetRoomVisibilityOnDirectoryJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            ~GetRoomVisibilityOnDirectoryJob() override;

            // Result properties

            const QString& visibility() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class SetRoomVisibilityOnDirectoryJob : public BaseJob
    {
        public:
            explicit SetRoomVisibilityOnDirectoryJob(const QString& roomId, const QString& visibility = {});
    };

    class GetPublicRoomsJob : public BaseJob
    {
        public:
            // Inner data structures

            struct PublicRoomsChunk
            {
                QStringList aliases;
                QString canonicalAlias;
                QString name;
                qint64 numJoinedMembers;
                QString roomId;
                QString topic;
                bool worldReadable;
                bool guestCanJoin;
                QString avatarUrl;
            };

            // Construction/destruction

            explicit GetPublicRoomsJob(int limit = {}, const QString& since = {}, const QString& server = {});

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetPublicRoomsJob. This function can be used when
             * a URL for GetPublicRoomsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, int limit = {}, const QString& since = {}, const QString& server = {});

            ~GetPublicRoomsJob() override;

            // Result properties

            const QVector<PublicRoomsChunk>& chunk() const;
            const QString& nextBatch() const;
            const QString& prevBatch() const;
            qint64 totalRoomCountEstimate() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class QueryPublicRoomsJob : public BaseJob
    {
        public:
            // Inner data structures

            struct Filter
            {
                QString genericSearchTerm;
            };

            struct PublicRoomsChunk
            {
                QStringList aliases;
                QString canonicalAlias;
                QString name;
                qint64 numJoinedMembers;
                QString roomId;
                QString topic;
                bool worldReadable;
                bool guestCanJoin;
                QString avatarUrl;
            };

            // Construction/destruction

            explicit QueryPublicRoomsJob(const QString& server = {}, int limit = {}, const QString& since = {}, const Filter& filter = {});
            ~QueryPublicRoomsJob() override;

            // Result properties

            const QVector<PublicRoomsChunk>& chunk() const;
            const QString& nextBatch() const;
            const QString& prevBatch() const;
            qint64 totalRoomCountEstimate() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

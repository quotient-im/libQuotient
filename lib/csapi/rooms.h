/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "events/roommemberevent.h"
#include "events/eventloader.h"
#include <QtCore/QHash>
#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class GetOneRoomEventJob : public BaseJob
    {
        public:
            explicit GetOneRoomEventJob(const QString& roomId, const QString& eventId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetOneRoomEventJob. This function can be used when
             * a URL for GetOneRoomEventJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventId);

            ~GetOneRoomEventJob() override;

            // Result properties

            EventPtr&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetRoomStateWithKeyJob : public BaseJob
    {
        public:
            explicit GetRoomStateWithKeyJob(const QString& roomId, const QString& eventType, const QString& stateKey);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetRoomStateWithKeyJob. This function can be used when
             * a URL for GetRoomStateWithKeyJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventType, const QString& stateKey);

            ~GetRoomStateWithKeyJob() override;

            // Result properties

            StateEventPtr&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetRoomStateByTypeJob : public BaseJob
    {
        public:
            explicit GetRoomStateByTypeJob(const QString& roomId, const QString& eventType);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetRoomStateByTypeJob. This function can be used when
             * a URL for GetRoomStateByTypeJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventType);

            ~GetRoomStateByTypeJob() override;

            // Result properties

            StateEventPtr&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetRoomStateJob : public BaseJob
    {
        public:
            explicit GetRoomStateJob(const QString& roomId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetRoomStateJob. This function can be used when
             * a URL for GetRoomStateJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            ~GetRoomStateJob() override;

            // Result properties

            StateEvents&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetMembersByRoomJob : public BaseJob
    {
        public:
            explicit GetMembersByRoomJob(const QString& roomId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetMembersByRoomJob. This function can be used when
             * a URL for GetMembersByRoomJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            ~GetMembersByRoomJob() override;

            // Result properties

            EventsArray<RoomMemberEvent>&& chunk();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class GetJoinedMembersByRoomJob : public BaseJob
    {
        public:
            // Inner data structures

            struct RoomMember
            {
                QString displayName;
                QString avatarUrl;
            };

            // Construction/destruction

            explicit GetJoinedMembersByRoomJob(const QString& roomId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetJoinedMembersByRoomJob. This function can be used when
             * a URL for GetJoinedMembersByRoomJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            ~GetJoinedMembersByRoomJob() override;

            // Result properties

            const QHash<QString, RoomMember>& joined() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

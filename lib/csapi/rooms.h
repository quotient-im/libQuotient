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

    /// Get a single event by event ID.
    /// 
    /// Get a single event based on ``roomId/eventId``. You must have permission to
    /// retrieve this event e.g. by being a member in the room for this event.
    class GetOneRoomEventJob : public BaseJob
    {
        public:
            /*! Get a single event by event ID.
             * \param roomId 
             *   The ID of the room the event is in.
             * \param eventId 
             *   The event ID to get.
             */
            explicit GetOneRoomEventJob(const QString& roomId, const QString& eventId);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetOneRoomEventJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventId);

            ~GetOneRoomEventJob() override;

            // Result properties

            /// The full event.
            EventPtr&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Get the state identified by the type and key.
    /// 
    /// Looks up the contents of a state event in a room. If the user is
    /// joined to the room then the state is taken from the current
    /// state of the room. If the user has left the room then the state is
    /// taken from the state of the room when they left.
    class GetRoomStateWithKeyJob : public BaseJob
    {
        public:
            /*! Get the state identified by the type and key.
             * \param roomId 
             *   The room to look up the state in.
             * \param eventType 
             *   The type of state to look up.
             * \param stateKey 
             *   The key of the state to look up.
             */
            explicit GetRoomStateWithKeyJob(const QString& roomId, const QString& eventType, const QString& stateKey);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetRoomStateWithKeyJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventType, const QString& stateKey);

            ~GetRoomStateWithKeyJob() override;

            // Result properties

            /// The content of the state event.
            StateEventPtr&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Get the state identified by the type, with the empty state key.
    /// 
    /// Looks up the contents of a state event in a room. If the user is
    /// joined to the room then the state is taken from the current
    /// state of the room. If the user has left the room then the state is
    /// taken from the state of the room when they left.
    /// 
    /// This looks up the state event with the empty state key.
    class GetRoomStateByTypeJob : public BaseJob
    {
        public:
            /*! Get the state identified by the type, with the empty state key.
             * \param roomId 
             *   The room to look up the state in.
             * \param eventType 
             *   The type of state to look up.
             */
            explicit GetRoomStateByTypeJob(const QString& roomId, const QString& eventType);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetRoomStateByTypeJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventType);

            ~GetRoomStateByTypeJob() override;

            // Result properties

            /// The content of the state event.
            StateEventPtr&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Get all state events in the current state of a room.
    /// 
    /// Get the state events for the current state of a room.
    class GetRoomStateJob : public BaseJob
    {
        public:
            /*! Get all state events in the current state of a room.
             * \param roomId 
             *   The room to look up the state for.
             */
            explicit GetRoomStateJob(const QString& roomId);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetRoomStateJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            ~GetRoomStateJob() override;

            // Result properties

            /// If the user is a member of the room this will be the
            /// current state of the room as a list of events. If the user
            /// has left the room then this will be the state of the room
            /// when they left as a list of events.
            StateEvents&& data();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Get the m.room.member events for the room.
    /// 
    /// Get the list of members for this room.
    class GetMembersByRoomJob : public BaseJob
    {
        public:
            /*! Get the m.room.member events for the room.
             * \param roomId 
             *   The room to get the member events for.
             */
            explicit GetMembersByRoomJob(const QString& roomId);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetMembersByRoomJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            ~GetMembersByRoomJob() override;

            // Result properties

            /// Get the list of members for this room.
            EventsArray<RoomMemberEvent>&& chunk();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Gets the list of currently joined users and their profile data.
    /// 
    /// This API returns a map of MXIDs to member info objects for members of the room. The current user must be in the room for it to work, unless it is an Application Service in which case any of the AS's users must be in the room. This API is primarily for Application Services and should be faster to respond than ``/members`` as it can be implemented more efficiently on the server.
    class GetJoinedMembersByRoomJob : public BaseJob
    {
        public:
            // Inner data structures

            /// This API returns a map of MXIDs to member info objects for members of the room. The current user must be in the room for it to work, unless it is an Application Service in which case any of the AS's users must be in the room. This API is primarily for Application Services and should be faster to respond than ``/members`` as it can be implemented more efficiently on the server.
            struct RoomMember
            {
                /// The display name of the user this object is representing.
                QString displayName;
                /// The mxc avatar url of the user this object is representing.
                QString avatarUrl;
            };

            // Construction/destruction

            /*! Gets the list of currently joined users and their profile data.
             * \param roomId 
             *   The room to get the members of.
             */
            explicit GetJoinedMembersByRoomJob(const QString& roomId);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetJoinedMembersByRoomJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

            ~GetJoinedMembersByRoomJob() override;

            // Result properties

            /// A map from user ID to a RoomMember object.
            const QHash<QString, RoomMember>& joined() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

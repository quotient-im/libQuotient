/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include <QtCore/QJsonObject>

namespace QMatrixClient
{
    // Operations

    /// Start the requesting user participating in a particular room.
    ///
    /// *Note that this API requires a room ID, not alias.* ``/join/{roomIdOrAlias}`` *exists if you have a room alias.*
    /// 
    /// This API starts a user participating in a particular room, if that user
    /// is allowed to participate in that room. After this call, the client is
    /// allowed to see all current state events in the room, and all subsequent
    /// events associated with the room until the user leaves the room.
    /// 
    /// After a user has joined a room, the room will appear as an entry in the
    /// response of the |/initialSync|_ and |/sync|_ APIs.
    /// 
    /// If a ``third_party_signed`` was supplied, the homeserver must verify
    /// that it matches a pending ``m.room.third_party_invite`` event in the
    /// room, and perform key validity checking if required by the event.
    class JoinRoomByIdJob : public BaseJob
    {
        public:
            // Inner data structures

            /// A signature of an ``m.third_party_invite`` token to prove that this user owns a third party identity which has been invited to the room.
            struct ThirdPartySigned
            {
                /// The Matrix ID of the user who issued the invite.
                QString sender;
                /// The Matrix ID of the invitee.
                QString mxid;
                /// The state key of the m.third_party_invite event.
                QString token;
                /// A signatures object containing a signature of the entire signed object.
                QJsonObject signatures;
            };

            // Construction/destruction

            /*! Start the requesting user participating in a particular room.
             * \param roomId
             *   The room identifier (not alias) to join.
             * \param thirdPartySigned
             *   A signature of an ``m.third_party_invite`` token to prove that this user owns a third party identity which has been invited to the room.
             */
            explicit JoinRoomByIdJob(const QString& roomId, const Omittable<ThirdPartySigned>& thirdPartySigned = none);
            ~JoinRoomByIdJob() override;

            // Result properties

            /// The joined room ID.
            const QString& roomId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Start the requesting user participating in a particular room.
    ///
    /// *Note that this API takes either a room ID or alias, unlike* ``/room/{roomId}/join``.
    /// 
    /// This API starts a user participating in a particular room, if that user
    /// is allowed to participate in that room. After this call, the client is
    /// allowed to see all current state events in the room, and all subsequent
    /// events associated with the room until the user leaves the room.
    /// 
    /// After a user has joined a room, the room will appear as an entry in the
    /// response of the |/initialSync|_ and |/sync|_ APIs.
    /// 
    /// If a ``third_party_signed`` was supplied, the homeserver must verify
    /// that it matches a pending ``m.room.third_party_invite`` event in the
    /// room, and perform key validity checking if required by the event.
    class JoinRoomJob : public BaseJob
    {
        public:
            // Inner data structures

            /// *Note that this API takes either a room ID or alias, unlike* ``/room/{roomId}/join``.
            /// 
            /// This API starts a user participating in a particular room, if that user
            /// is allowed to participate in that room. After this call, the client is
            /// allowed to see all current state events in the room, and all subsequent
            /// events associated with the room until the user leaves the room.
            /// 
            /// After a user has joined a room, the room will appear as an entry in the
            /// response of the |/initialSync|_ and |/sync|_ APIs.
            /// 
            /// If a ``third_party_signed`` was supplied, the homeserver must verify
            /// that it matches a pending ``m.room.third_party_invite`` event in the
            /// room, and perform key validity checking if required by the event.
            struct Signed
            {
                /// The Matrix ID of the user who issued the invite.
                QString sender;
                /// The Matrix ID of the invitee.
                QString mxid;
                /// The state key of the m.third_party_invite event.
                QString token;
                /// A signatures object containing a signature of the entire signed object.
                QJsonObject signatures;
            };

            /// A signature of an ``m.third_party_invite`` token to prove that this user owns a third party identity which has been invited to the room.
            struct ThirdPartySigned
            {
                /// A signature of an ``m.third_party_invite`` token to prove that this user owns a third party identity which has been invited to the room.
                Signed signedData;
            };

            // Construction/destruction

            /*! Start the requesting user participating in a particular room.
             * \param roomIdOrAlias
             *   The room identifier or alias to join.
             * \param serverName
             *   The servers to attempt to join the room through. One of the servers
             *   must be participating in the room.
             * \param thirdPartySigned
             *   A signature of an ``m.third_party_invite`` token to prove that this user owns a third party identity which has been invited to the room.
             */
            explicit JoinRoomJob(const QString& roomIdOrAlias, const QStringList& serverName = {}, const Omittable<ThirdPartySigned>& thirdPartySigned = none);
            ~JoinRoomJob() override;

            // Result properties

            /// The joined room ID.
            const QString& roomId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>
#include "converters.h"
#include <QtCore/QVector>

namespace QMatrixClient
{
    // Operations

    /// Create a new room
    /// 
    /// Create a new room with various configuration options.
    /// 
    /// The server MUST apply the normal state resolution rules when creating
    /// the new room, including checking power levels for each event. It MUST
    /// apply the events implied by the request in the following order:
    /// 
    /// 0. A default ``m.room.power_levels`` event, giving the room creator
    ///    (and not other members) permission to send state events.
    /// 
    /// 1. Events set by the ``preset``.
    /// 
    /// 2. Events listed in ``initial_state``, in the order that they are
    ///    listed.
    /// 
    /// 3. Events implied by ``name`` and ``topic``.
    /// 
    /// 4. Invite events implied by ``invite`` and ``invite_3pid``.
    /// 
    /// The available presets do the following with respect to room state:
    /// 
    /// ========================  ==============  ======================  ================  =========
    ///          Preset           ``join_rules``  ``history_visibility``  ``guest_access``  Other
    /// ========================  ==============  ======================  ================  =========
    /// ``private_chat``          ``invite``      ``shared``              ``can_join``      
    /// ``trusted_private_chat``  ``invite``      ``shared``              ``can_join``      All invitees are given the same power level as the room creator.
    /// ``public_chat``           ``public``      ``shared``              ``forbidden``     
    /// ========================  ==============  ======================  ================  =========
    class CreateRoomJob : public BaseJob
    {
        public:
            // Inner data structures

            /// Create a new room with various configuration options.
            /// 
            /// The server MUST apply the normal state resolution rules when creating
            /// the new room, including checking power levels for each event. It MUST
            /// apply the events implied by the request in the following order:
            /// 
            /// 0. A default ``m.room.power_levels`` event, giving the room creator
            ///    (and not other members) permission to send state events.
            /// 
            /// 1. Events set by the ``preset``.
            /// 
            /// 2. Events listed in ``initial_state``, in the order that they are
            ///    listed.
            /// 
            /// 3. Events implied by ``name`` and ``topic``.
            /// 
            /// 4. Invite events implied by ``invite`` and ``invite_3pid``.
            /// 
            /// The available presets do the following with respect to room state:
            /// 
            /// ========================  ==============  ======================  ================  =========
            ///          Preset           ``join_rules``  ``history_visibility``  ``guest_access``  Other
            /// ========================  ==============  ======================  ================  =========
            /// ``private_chat``          ``invite``      ``shared``              ``can_join``      
            /// ``trusted_private_chat``  ``invite``      ``shared``              ``can_join``      All invitees are given the same power level as the room creator.
            /// ``public_chat``           ``public``      ``shared``              ``forbidden``     
            /// ========================  ==============  ======================  ================  =========
            struct Invite3pid
            {
                /// The hostname+port of the identity server which should be used for third party identifier lookups.
                QString idServer;
                /// The kind of address being passed in the address field, for example ``email``.
                QString medium;
                /// The invitee's third party identifier.
                QString address;
            };

            /// Create a new room with various configuration options.
            /// 
            /// The server MUST apply the normal state resolution rules when creating
            /// the new room, including checking power levels for each event. It MUST
            /// apply the events implied by the request in the following order:
            /// 
            /// 0. A default ``m.room.power_levels`` event, giving the room creator
            ///    (and not other members) permission to send state events.
            /// 
            /// 1. Events set by the ``preset``.
            /// 
            /// 2. Events listed in ``initial_state``, in the order that they are
            ///    listed.
            /// 
            /// 3. Events implied by ``name`` and ``topic``.
            /// 
            /// 4. Invite events implied by ``invite`` and ``invite_3pid``.
            /// 
            /// The available presets do the following with respect to room state:
            /// 
            /// ========================  ==============  ======================  ================  =========
            ///          Preset           ``join_rules``  ``history_visibility``  ``guest_access``  Other
            /// ========================  ==============  ======================  ================  =========
            /// ``private_chat``          ``invite``      ``shared``              ``can_join``      
            /// ``trusted_private_chat``  ``invite``      ``shared``              ``can_join``      All invitees are given the same power level as the room creator.
            /// ``public_chat``           ``public``      ``shared``              ``forbidden``     
            /// ========================  ==============  ======================  ================  =========
            struct StateEvent
            {
                /// The type of event to send.
                QString type;
                /// The state_key of the state event. Defaults to an empty string.
                QString stateKey;
                /// The content of the event.
                QJsonObject content;
            };

            // Construction/destruction

            /*! Create a new room
             * \param visibility 
             *   A ``public`` visibility indicates that the room will be shown
             *   in the published room list. A ``private`` visibility will hide
             *   the room from the published room list. Rooms default to
             *   ``private`` visibility if this key is not included. NB: This
             *   should not be confused with ``join_rules`` which also uses the
             *   word ``public``.
             * \param roomAliasName 
             *   The desired room alias **local part**. If this is included, a
             *   room alias will be created and mapped to the newly created
             *   room. The alias will belong on the *same* homeserver which
             *   created the room. For example, if this was set to "foo" and
             *   sent to the homeserver "example.com" the complete room alias
             *   would be ``#foo:example.com``.
             * \param name 
             *   If this is included, an ``m.room.name`` event will be sent
             *   into the room to indicate the name of the room. See Room
             *   Events for more information on ``m.room.name``.
             * \param topic 
             *   If this is included, an ``m.room.topic`` event will be sent
             *   into the room to indicate the topic for the room. See Room
             *   Events for more information on ``m.room.topic``.
             * \param invite 
             *   A list of user IDs to invite to the room. This will tell the
             *   server to invite everyone in the list to the newly created room.
             * \param invite3pid 
             *   A list of objects representing third party IDs to invite into
             *   the room.
             * \param creationContent 
             *   Extra keys to be added to the content of the ``m.room.create``.
             *   The server will clobber the following keys: ``creator``. Future
             *   versions of the specification may allow the server to clobber
             *   other keys.
             * \param initialState 
             *   A list of state events to set in the new room. This allows
             *   the user to override the default state events set in the new
             *   room. The expected format of the state events are an object
             *   with type, state_key and content keys set.
             *   
             *   Takes precedence over events set by ``preset``, but gets
             *   overriden by ``name`` and ``topic`` keys.
             * \param preset 
             *   Convenience parameter for setting various default state events
             *   based on a preset.
             * \param isDirect 
             *   This flag makes the server set the ``is_direct`` flag on the
             *   ``m.room.member`` events sent to the users in ``invite`` and
             *   ``invite_3pid``. See `Direct Messaging`_ for more information.
             * \param guestCanJoin 
             *   Allows guests to join the room. See `Guest Access`_ for more information.
             */
            explicit CreateRoomJob(const QString& visibility = {}, const QString& roomAliasName = {}, const QString& name = {}, const QString& topic = {}, const QStringList& invite = {}, const QVector<Invite3pid>& invite3pid = {}, const QJsonObject& creationContent = {}, const QVector<StateEvent>& initialState = {}, const QString& preset = {}, bool isDirect = false, bool guestCanJoin = false);
            ~CreateRoomJob() override;

            // Result properties

            /// The created room's ID.
            const QString& roomId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

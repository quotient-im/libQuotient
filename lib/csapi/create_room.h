/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Create a new room
 *
 * Create a new room with various configuration options.
 *
 * The server MUST apply the normal state resolution rules when creating
 * the new room, including checking power levels for each event. It MUST
 * apply the events implied by the request in the following order:
 *
 * 1. The `m.room.create` event itself. Must be the first event in the
 *    room.
 *
 * 2. An `m.room.member` event for the creator to join the room. This is
 *    needed so the remaining events can be sent.
 *
 * 3. A default `m.room.power_levels` event, giving the room creator
 *    (and not other members) permission to send state events. Overridden
 *    by the `power_level_content_override` parameter.
 *
 * 4. Events set by the `preset`. Currently these are the `m.room.join_rules`,
 *    `m.room.history_visibility`, and `m.room.guest_access` state events.
 *
 * 5. Events listed in `initial_state`, in the order that they are
 *    listed.
 *
 * 6. Events implied by `name` and `topic` (`m.room.name` and `m.room.topic`
 *    state events).
 *
 * 7. Invite events implied by `invite` and `invite_3pid` (`m.room.member` with
 *    `membership: invite` and `m.room.third_party_invite`).
 *
 * The available presets do the following with respect to room state:
 *
 * | Preset                 | `join_rules` | `history_visibility` |
 * `guest_access` | Other |
 * |------------------------|--------------|----------------------|----------------|-------|
 * | `private_chat`         | `invite`     | `shared`             | `can_join`
 * |       | | `trusted_private_chat` | `invite`     | `shared`             |
 * `can_join`     | All invitees are given the same power level as the room
 * creator. | | `public_chat`          | `public`     | `shared`             |
 * `forbidden`    |       |
 *
 * The server will create a `m.room.create` event in the room with the
 * requesting user as the creator, alongside other keys provided in the
 * `creation_content`.
 */
class CreateRoomJob : public BaseJob {
public:
    // Inner data structures

    /// Create a new room with various configuration options.
    ///
    /// The server MUST apply the normal state resolution rules when creating
    /// the new room, including checking power levels for each event. It MUST
    /// apply the events implied by the request in the following order:
    ///
    /// 1. The `m.room.create` event itself. Must be the first event in the
    ///    room.
    ///
    /// 2. An `m.room.member` event for the creator to join the room. This is
    ///    needed so the remaining events can be sent.
    ///
    /// 3. A default `m.room.power_levels` event, giving the room creator
    ///    (and not other members) permission to send state events. Overridden
    ///    by the `power_level_content_override` parameter.
    ///
    /// 4. Events set by the `preset`. Currently these are the
    /// `m.room.join_rules`,
    ///    `m.room.history_visibility`, and `m.room.guest_access` state events.
    ///
    /// 5. Events listed in `initial_state`, in the order that they are
    ///    listed.
    ///
    /// 6. Events implied by `name` and `topic` (`m.room.name` and `m.room.topic`
    ///    state events).
    ///
    /// 7. Invite events implied by `invite` and `invite_3pid` (`m.room.member`
    /// with
    ///    `membership: invite` and `m.room.third_party_invite`).
    ///
    /// The available presets do the following with respect to room state:
    ///
    /// | Preset                 | `join_rules` | `history_visibility` |
    /// `guest_access` | Other |
    /// |------------------------|--------------|----------------------|----------------|-------|
    /// | `private_chat`         | `invite`     | `shared`             |
    /// `can_join`     |       | | `trusted_private_chat` | `invite`     |
    /// `shared`             | `can_join`     | All invitees are given the same
    /// power level as the room creator. | | `public_chat`          | `public`
    /// | `shared`             | `forbidden`    |       |
    ///
    /// The server will create a `m.room.create` event in the room with the
    /// requesting user as the creator, alongside other keys provided in the
    /// `creation_content`.
    struct Invite3pid {
        /// The hostname+port of the identity server which should be used for
        /// third party identifier lookups.
        QString idServer;
        /// An access token previously registered with the identity server.
        /// Servers can treat this as optional to distinguish between
        /// r0.5-compatible clients and this specification version.
        QString idAccessToken;
        /// The kind of address being passed in the address field, for example
        /// `email`.
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
    /// 1. The `m.room.create` event itself. Must be the first event in the
    ///    room.
    ///
    /// 2. An `m.room.member` event for the creator to join the room. This is
    ///    needed so the remaining events can be sent.
    ///
    /// 3. A default `m.room.power_levels` event, giving the room creator
    ///    (and not other members) permission to send state events. Overridden
    ///    by the `power_level_content_override` parameter.
    ///
    /// 4. Events set by the `preset`. Currently these are the
    /// `m.room.join_rules`,
    ///    `m.room.history_visibility`, and `m.room.guest_access` state events.
    ///
    /// 5. Events listed in `initial_state`, in the order that they are
    ///    listed.
    ///
    /// 6. Events implied by `name` and `topic` (`m.room.name` and `m.room.topic`
    ///    state events).
    ///
    /// 7. Invite events implied by `invite` and `invite_3pid` (`m.room.member`
    /// with
    ///    `membership: invite` and `m.room.third_party_invite`).
    ///
    /// The available presets do the following with respect to room state:
    ///
    /// | Preset                 | `join_rules` | `history_visibility` |
    /// `guest_access` | Other |
    /// |------------------------|--------------|----------------------|----------------|-------|
    /// | `private_chat`         | `invite`     | `shared`             |
    /// `can_join`     |       | | `trusted_private_chat` | `invite`     |
    /// `shared`             | `can_join`     | All invitees are given the same
    /// power level as the room creator. | | `public_chat`          | `public`
    /// | `shared`             | `forbidden`    |       |
    ///
    /// The server will create a `m.room.create` event in the room with the
    /// requesting user as the creator, alongside other keys provided in the
    /// `creation_content`.
    struct StateEvent {
        /// The type of event to send.
        QString type;
        /// The state_key of the state event. Defaults to an empty string.
        QString stateKey;
        /// The content of the event.
        QJsonObject content;
    };

    // Construction/destruction

    /*! \brief Create a new room
     *
     * \param visibility
     *   A `public` visibility indicates that the room will be shown
     *   in the published room list. A `private` visibility will hide
     *   the room from the published room list. Rooms default to
     *   `private` visibility if this key is not included. NB: This
     *   should not be confused with `join_rules` which also uses the
     *   word `public`.
     *
     * \param roomAliasName
     *   The desired room alias **local part**. If this is included, a
     *   room alias will be created and mapped to the newly created
     *   room. The alias will belong on the *same* homeserver which
     *   created the room. For example, if this was set to "foo" and
     *   sent to the homeserver "example.com" the complete room alias
     *   would be `#foo:example.com`.
     *
     *   The complete room alias will become the canonical alias for
     *   the room.
     *
     * \param name
     *   If this is included, an `m.room.name` event will be sent
     *   into the room to indicate the name of the room. See Room
     *   Events for more information on `m.room.name`.
     *
     * \param topic
     *   If this is included, an `m.room.topic` event will be sent
     *   into the room to indicate the topic for the room. See Room
     *   Events for more information on `m.room.topic`.
     *
     * \param invite
     *   A list of user IDs to invite to the room. This will tell the
     *   server to invite everyone in the list to the newly created room.
     *
     * \param invite3pid
     *   A list of objects representing third party IDs to invite into
     *   the room.
     *
     * \param roomVersion
     *   The room version to set for the room. If not provided, the homeserver
     * is to use its configured default. If provided, the homeserver will return
     * a 400 error with the errcode `M_UNSUPPORTED_ROOM_VERSION` if it does not
     *   support the room version.
     *
     * \param creationContent
     *   Extra keys, such as `m.federate`, to be added to the content
     *   of the [`m.room.create`](client-server-api/#mroomcreate) event. The
     * server will clobber the following keys: `creator`, `room_version`. Future
     * versions of the specification may allow the server to clobber other keys.
     *
     * \param initialState
     *   A list of state events to set in the new room. This allows
     *   the user to override the default state events set in the new
     *   room. The expected format of the state events are an object
     *   with type, state_key and content keys set.
     *
     *   Takes precedence over events set by `preset`, but gets
     *   overriden by `name` and `topic` keys.
     *
     * \param preset
     *   Convenience parameter for setting various default state events
     *   based on a preset.
     *
     *   If unspecified, the server should use the `visibility` to determine
     *   which preset to use. A visbility of `public` equates to a preset of
     *   `public_chat` and `private` visibility equates to a preset of
     *   `private_chat`.
     *
     * \param isDirect
     *   This flag makes the server set the `is_direct` flag on the
     *   `m.room.member` events sent to the users in `invite` and
     *   `invite_3pid`. See [Direct
     * Messaging](/client-server-api/#direct-messaging) for more information.
     *
     * \param powerLevelContentOverride
     *   The power level content to override in the default power level
     *   event. This object is applied on top of the generated
     *   [`m.room.power_levels`](client-server-api/#mroompower_levels)
     *   event content prior to it being sent to the room. Defaults to
     *   overriding nothing.
     */
    explicit CreateRoomJob(const QString& visibility = {},
                           const QString& roomAliasName = {},
                           const QString& name = {}, const QString& topic = {},
                           const QStringList& invite = {},
                           const QVector<Invite3pid>& invite3pid = {},
                           const QString& roomVersion = {},
                           const QJsonObject& creationContent = {},
                           const QVector<StateEvent>& initialState = {},
                           const QString& preset = {},
                           Omittable<bool> isDirect = none,
                           const QJsonObject& powerLevelContentOverride = {});

    // Result properties

    /// The created room's ID.
    QString roomId() const { return loadFromJson<QString>("room_id"_ls); }
};

template <>
struct JsonObjectConverter<CreateRoomJob::Invite3pid> {
    static void dumpTo(QJsonObject& jo, const CreateRoomJob::Invite3pid& pod)
    {
        addParam<>(jo, QStringLiteral("id_server"), pod.idServer);
        addParam<>(jo, QStringLiteral("id_access_token"), pod.idAccessToken);
        addParam<>(jo, QStringLiteral("medium"), pod.medium);
        addParam<>(jo, QStringLiteral("address"), pod.address);
    }
};

template <>
struct JsonObjectConverter<CreateRoomJob::StateEvent> {
    static void dumpTo(QJsonObject& jo, const CreateRoomJob::StateEvent& pod)
    {
        addParam<>(jo, QStringLiteral("type"), pod.type);
        addParam<IfNotEmpty>(jo, QStringLiteral("state_key"), pod.stateKey);
        addParam<>(jo, QStringLiteral("content"), pod.content);
    }
};

} // namespace Quotient

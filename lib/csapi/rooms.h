/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "events/eventloader.h"
#include "events/roommemberevent.h"
#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Get a single event by event ID.
 *
 * Get a single event based on `roomId/eventId`. You must have permission to
 * retrieve this event e.g. by being a member in the room for this event.
 */
class GetOneRoomEventJob : public BaseJob {
public:
    /*! \brief Get a single event by event ID.
     *
     * \param roomId
     *   The ID of the room the event is in.
     *
     * \param eventId
     *   The event ID to get.
     */
    explicit GetOneRoomEventJob(const QString& roomId, const QString& eventId);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetOneRoomEventJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& eventId);

    // Result properties

    /// The full event.
    EventPtr event() { return fromJson<EventPtr>(jsonData()); }
};

/*! \brief Get the state identified by the type and key.
 *
 * Looks up the contents of a state event in a room. If the user is
 * joined to the room then the state is taken from the current
 * state of the room. If the user has left the room then the state is
 * taken from the state of the room when they left.
 */
class GetRoomStateWithKeyJob : public BaseJob {
public:
    /*! \brief Get the state identified by the type and key.
     *
     * \param roomId
     *   The room to look up the state in.
     *
     * \param eventType
     *   The type of state to look up.
     *
     * \param stateKey
     *   The key of the state to look up. Defaults to an empty string. When
     *   an empty string, the trailing slash on this endpoint is optional.
     */
    explicit GetRoomStateWithKeyJob(const QString& roomId,
                                    const QString& eventType,
                                    const QString& stateKey);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRoomStateWithKeyJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& eventType,
                               const QString& stateKey);
};

/*! \brief Get all state events in the current state of a room.
 *
 * Get the state events for the current state of a room.
 */
class GetRoomStateJob : public BaseJob {
public:
    /*! \brief Get all state events in the current state of a room.
     *
     * \param roomId
     *   The room to look up the state for.
     */
    explicit GetRoomStateJob(const QString& roomId);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRoomStateJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

    // Result properties

    /// The current state of the room
    StateEvents events() { return fromJson<StateEvents>(jsonData()); }
};

/*! \brief Get the m.room.member events for the room.
 *
 * Get the list of members for this room.
 */
class GetMembersByRoomJob : public BaseJob {
public:
    /*! \brief Get the m.room.member events for the room.
     *
     * \param roomId
     *   The room to get the member events for.
     *
     * \param at
     *   The point in time (pagination token) to return members for in the room.
     *   This token can be obtained from a `prev_batch` token returned for
     *   each room by the sync API. Defaults to the current state of the room,
     *   as determined by the server.
     *
     * \param membership
     *   The kind of membership to filter for. Defaults to no filtering if
     *   unspecified. When specified alongside `not_membership`, the two
     *   parameters create an 'or' condition: either the membership *is*
     *   the same as `membership` **or** *is not* the same as `not_membership`.
     *
     * \param notMembership
     *   The kind of membership to exclude from the results. Defaults to no
     *   filtering if unspecified.
     */
    explicit GetMembersByRoomJob(const QString& roomId, const QString& at = {},
                                 const QString& membership = {},
                                 const QString& notMembership = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetMembersByRoomJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& at = {},
                               const QString& membership = {},
                               const QString& notMembership = {});

    // Result properties

    /// Get the list of members for this room.
    EventsArray<RoomMemberEvent> chunk()
    {
        return takeFromJson<EventsArray<RoomMemberEvent>>("chunk"_ls);
    }
};

/*! \brief Gets the list of currently joined users and their profile data.
 *
 * This API returns a map of MXIDs to member info objects for members of the
 * room. The current user must be in the room for it to work, unless it is an
 * Application Service in which case any of the AS's users must be in the room.
 * This API is primarily for Application Services and should be faster to
 * respond than `/members` as it can be implemented more efficiently on the
 * server.
 */
class GetJoinedMembersByRoomJob : public BaseJob {
public:
    // Inner data structures

    /// This API returns a map of MXIDs to member info objects for members of
    /// the room. The current user must be in the room for it to work, unless it
    /// is an Application Service in which case any of the AS's users must be in
    /// the room. This API is primarily for Application Services and should be
    /// faster to respond than `/members` as it can be implemented more
    /// efficiently on the server.
    struct RoomMember {
        /// The display name of the user this object is representing.
        QString displayName;
        /// The mxc avatar url of the user this object is representing.
        QUrl avatarUrl;
    };

    // Construction/destruction

    /*! \brief Gets the list of currently joined users and their profile data.
     *
     * \param roomId
     *   The room to get the members of.
     */
    explicit GetJoinedMembersByRoomJob(const QString& roomId);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetJoinedMembersByRoomJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

    // Result properties

    /// A map from user ID to a RoomMember object.
    QHash<QString, RoomMember> joined() const
    {
        return loadFromJson<QHash<QString, RoomMember>>("joined"_ls);
    }
};

template <>
struct JsonObjectConverter<GetJoinedMembersByRoomJob::RoomMember> {
    static void fillFrom(const QJsonObject& jo,
                         GetJoinedMembersByRoomJob::RoomMember& result)
    {
        fromJson(jo.value("display_name"_ls), result.displayName);
        fromJson(jo.value("avatar_url"_ls), result.avatarUrl);
    }
};

} // namespace Quotient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../events/stateevent.h"
#include "../jobs/basejob.h"

namespace Quotient {

/*! \brief Retrieve a portion of a space tree.
 *
 * Paginates over the space tree in a depth-first manner to locate child rooms
 * of a given space.
 *
 * Where a child room is unknown to the local server, federation is used to fill
 * in the details. The servers listed in the `via` array should be contacted to
 * attempt to fill in missing rooms.
 *
 * Only [`m.space.child`](#mspacechild) state events of the room are considered.
 * Invalid child rooms and parent events are not covered by this endpoint.
 */
class QUOTIENT_API GetSpaceHierarchyJob : public BaseJob {
public:
    // Inner data structures

    /// Paginates over the space tree in a depth-first manner to locate child
    /// rooms of a given space.
    ///
    /// Where a child room is unknown to the local server, federation is used to
    /// fill in the details. The servers listed in the `via` array should be
    /// contacted to attempt to fill in missing rooms.
    ///
    /// Only [`m.space.child`](#mspacechild) state events of the room are
    /// considered. Invalid child rooms and parent events are not covered by
    /// this endpoint.
    struct ChildRoomsChunk {
        /// The canonical alias of the room, if any.
        QString canonicalAlias;
        /// The name of the room, if any.
        QString name;
        /// The number of members joined to the room.
        int numJoinedMembers;
        /// The ID of the room.
        QString roomId;
        /// The topic of the room, if any.
        QString topic;
        /// Whether the room may be viewed by guest users without joining.
        bool worldReadable;
        /// Whether guest users may join the room and participate in it.
        /// If they can, they will be subject to ordinary power level
        /// rules like any other user.
        bool guestCanJoin;
        /// The URL for the room's avatar, if one is set.
        QUrl avatarUrl;
        /// The room's join rule. When not present, the room is assumed to
        /// be `public`.
        QString joinRule;
        /// The `type` of room (from
        /// [`m.room.create`](/client-server-api/#mroomcreate)), if any.
        QString roomType;
        /// The [`m.space.child`](#mspacechild) events of the space-room,
        /// represented as [Stripped State Events](#stripped-state) with an
        /// added `origin_server_ts` key.
        ///
        /// If the room is not a space-room, this should be empty.
        StateEvents childrenState;
    };

    // Construction/destruction

    /*! \brief Retrieve a portion of a space tree.
     *
     * \param roomId
     *   The room ID of the space to get a hierarchy for.
     *
     * \param suggestedOnly
     *   Optional (default `false`) flag to indicate whether or not the server
     * should only consider suggested rooms. Suggested rooms are annotated in
     * their [`m.space.child`](#mspacechild) event contents.
     *
     * \param limit
     *   Optional limit for the maximum number of rooms to include per response.
     * Must be an integer greater than zero.
     *
     *   Servers should apply a default value, and impose a maximum value to
     * avoid resource exhaustion.
     *
     * \param maxDepth
     *   Optional limit for how far to go into the space. Must be a non-negative
     * integer.
     *
     *   When reached, no further child rooms will be returned.
     *
     *   Servers should apply a default value, and impose a maximum value to
     * avoid resource exhaustion.
     *
     * \param from
     *   A pagination token from a previous result. If specified, `max_depth`
     * and `suggested_only` cannot be changed from the first request.
     */
    explicit GetSpaceHierarchyJob(const QString& roomId,
                                  Omittable<bool> suggestedOnly = none,
                                  Omittable<int> limit = none,
                                  Omittable<int> maxDepth = none,
                                  const QString& from = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetSpaceHierarchyJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               Omittable<bool> suggestedOnly = none,
                               Omittable<int> limit = none,
                               Omittable<int> maxDepth = none,
                               const QString& from = {});

    // Result properties

    /// The rooms for the current page, with the current filters.
    std::vector<ChildRoomsChunk> rooms()
    {
        return takeFromJson<std::vector<ChildRoomsChunk>>("rooms"_ls);
    }

    /// A token to supply to `from` to keep paginating the responses. Not
    /// present when there are no further results.
    QString nextBatch() const { return loadFromJson<QString>("next_batch"_ls); }
};

template <>
struct JsonObjectConverter<GetSpaceHierarchyJob::ChildRoomsChunk> {
    static void fillFrom(const QJsonObject& jo,
                         GetSpaceHierarchyJob::ChildRoomsChunk& result)
    {
        fromJson(jo.value("canonical_alias"_ls), result.canonicalAlias);
        fromJson(jo.value("name"_ls), result.name);
        fromJson(jo.value("num_joined_members"_ls), result.numJoinedMembers);
        fromJson(jo.value("room_id"_ls), result.roomId);
        fromJson(jo.value("topic"_ls), result.topic);
        fromJson(jo.value("world_readable"_ls), result.worldReadable);
        fromJson(jo.value("guest_can_join"_ls), result.guestCanJoin);
        fromJson(jo.value("avatar_url"_ls), result.avatarUrl);
        fromJson(jo.value("join_rule"_ls), result.joinRule);
        fromJson(jo.value("room_type"_ls), result.roomType);
        fromJson(jo.value("children_state"_ls), result.childrenState);
    }
};

} // namespace Quotient

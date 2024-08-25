// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/public_rooms_response.h>

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Gets the visibility of a room in the directory
//!
//! Gets the visibility of a given room on the server's public room directory.
class QUOTIENT_API GetRoomVisibilityOnDirectoryJob : public BaseJob {
public:
    //! \param roomId
    //!   The room ID.
    explicit GetRoomVisibilityOnDirectoryJob(const QString& roomId);

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetRoomVisibilityOnDirectoryJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, const QString& roomId);

    // Result properties

    //! The visibility of the room in the directory.
    QString visibility() const { return loadFromJson<QString>("visibility"_L1); }
};

inline auto collectResponse(const GetRoomVisibilityOnDirectoryJob* job)
{
    return job->visibility();
}

//! \brief Sets the visibility of a room in the room directory
//!
//! Sets the visibility of a given room in the server's public room
//! directory.
//!
//! Servers may choose to implement additional access control checks
//! here, for instance that room visibility can only be changed by
//! the room creator or a server administrator.
class QUOTIENT_API SetRoomVisibilityOnDirectoryJob : public BaseJob {
public:
    //! \param roomId
    //!   The room ID.
    //!
    //! \param visibility
    //!   The new visibility setting for the room.
    //!   Defaults to 'public'.
    explicit SetRoomVisibilityOnDirectoryJob(const QString& roomId, const QString& visibility = {});
};

//! \brief Lists the public rooms on the server.
//!
//! Lists the public rooms on the server.
//!
//! This API returns paginated responses. The rooms are ordered by the number
//! of joined members, with the largest rooms first.
class QUOTIENT_API GetPublicRoomsJob : public BaseJob {
public:
    //! \param limit
    //!   Limit the number of results returned.
    //!
    //! \param since
    //!   A pagination token from a previous request, allowing clients to
    //!   get the next (or previous) batch of rooms.
    //!   The direction of pagination is specified solely by which token
    //!   is supplied, rather than via an explicit flag.
    //!
    //! \param server
    //!   The server to fetch the public room lists from. Defaults to the
    //!   local server. Case sensitive.
    explicit GetPublicRoomsJob(std::optional<int> limit = std::nullopt, const QString& since = {},
                               const QString& server = {});

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetPublicRoomsJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(const HomeserverData& hsData, std::optional<int> limit = std::nullopt,
                               const QString& since = {}, const QString& server = {});

    // Result properties

    //! A paginated chunk of public rooms.
    QVector<PublicRoomsChunk> chunk() const
    {
        return loadFromJson<QVector<PublicRoomsChunk>>("chunk"_L1);
    }

    //! A pagination token for the response. The absence of this token
    //! means there are no more results to fetch and the client should
    //! stop paginating.
    QString nextBatch() const { return loadFromJson<QString>("next_batch"_L1); }

    //! A pagination token that allows fetching previous results. The
    //! absence of this token means there are no results before this
    //! batch, i.e. this is the first batch.
    QString prevBatch() const { return loadFromJson<QString>("prev_batch"_L1); }

    //! An estimate on the total number of public rooms, if the
    //! server has an estimate.
    std::optional<int> totalRoomCountEstimate() const
    {
        return loadFromJson<std::optional<int>>("total_room_count_estimate"_L1);
    }

    struct Response {
        //! A paginated chunk of public rooms.
        QVector<PublicRoomsChunk> chunk{};

        //! A pagination token for the response. The absence of this token
        //! means there are no more results to fetch and the client should
        //! stop paginating.
        QString nextBatch{};

        //! A pagination token that allows fetching previous results. The
        //! absence of this token means there are no results before this
        //! batch, i.e. this is the first batch.
        QString prevBatch{};

        //! An estimate on the total number of public rooms, if the
        //! server has an estimate.
        std::optional<int> totalRoomCountEstimate{};
    };
};

template <std::derived_from<GetPublicRoomsJob> JobT>
constexpr inline auto doCollectResponse<JobT> = [](JobT* j) -> GetPublicRoomsJob::Response {
    return { j->chunk(), j->nextBatch(), j->prevBatch(), j->totalRoomCountEstimate() };
};

//! \brief Lists the public rooms on the server with optional filter.
//!
//! Lists the public rooms on the server, with optional filter.
//!
//! This API returns paginated responses. The rooms are ordered by the number
//! of joined members, with the largest rooms first.
class QUOTIENT_API QueryPublicRoomsJob : public BaseJob {
public:
    // Inner data structures

    //! Filter to apply to the results.
    struct QUOTIENT_API Filter {
        //! An optional string to search for in the room metadata, e.g. name,
        //! topic, canonical alias, etc.
        QString genericSearchTerm{};

        //! An optional list of [room types](/client-server-api/#types) to search
        //! for. To include rooms without a room type, specify `null` within this
        //! list. When not specified, all applicable rooms (regardless of type)
        //! are returned.
        QStringList roomTypes{};
    };

    // Construction/destruction

    //! \param server
    //!   The server to fetch the public room lists from. Defaults to the
    //!   local server. Case sensitive.
    //!
    //! \param limit
    //!   Limit the number of results returned.
    //!
    //! \param since
    //!   A pagination token from a previous request, allowing clients
    //!   to get the next (or previous) batch of rooms.  The direction
    //!   of pagination is specified solely by which token is supplied,
    //!   rather than via an explicit flag.
    //!
    //! \param filter
    //!   Filter to apply to the results.
    //!
    //! \param includeAllNetworks
    //!   Whether or not to include all known networks/protocols from
    //!   application services on the homeserver. Defaults to false.
    //!
    //! \param thirdPartyInstanceId
    //!   The specific third-party network/protocol to request from the
    //!   homeserver. Can only be used if `include_all_networks` is false.
    explicit QueryPublicRoomsJob(const QString& server = {},
                                 std::optional<int> limit = std::nullopt, const QString& since = {},
                                 const std::optional<Filter>& filter = std::nullopt,
                                 std::optional<bool> includeAllNetworks = std::nullopt,
                                 const QString& thirdPartyInstanceId = {});

    // Result properties

    //! A paginated chunk of public rooms.
    QVector<PublicRoomsChunk> chunk() const
    {
        return loadFromJson<QVector<PublicRoomsChunk>>("chunk"_L1);
    }

    //! A pagination token for the response. The absence of this token
    //! means there are no more results to fetch and the client should
    //! stop paginating.
    QString nextBatch() const { return loadFromJson<QString>("next_batch"_L1); }

    //! A pagination token that allows fetching previous results. The
    //! absence of this token means there are no results before this
    //! batch, i.e. this is the first batch.
    QString prevBatch() const { return loadFromJson<QString>("prev_batch"_L1); }

    //! An estimate on the total number of public rooms, if the
    //! server has an estimate.
    std::optional<int> totalRoomCountEstimate() const
    {
        return loadFromJson<std::optional<int>>("total_room_count_estimate"_L1);
    }

    struct Response {
        //! A paginated chunk of public rooms.
        QVector<PublicRoomsChunk> chunk{};

        //! A pagination token for the response. The absence of this token
        //! means there are no more results to fetch and the client should
        //! stop paginating.
        QString nextBatch{};

        //! A pagination token that allows fetching previous results. The
        //! absence of this token means there are no results before this
        //! batch, i.e. this is the first batch.
        QString prevBatch{};

        //! An estimate on the total number of public rooms, if the
        //! server has an estimate.
        std::optional<int> totalRoomCountEstimate{};
    };
};

template <std::derived_from<QueryPublicRoomsJob> JobT>
constexpr inline auto doCollectResponse<JobT> = [](JobT* j) -> QueryPublicRoomsJob::Response {
    return { j->chunk(), j->nextBatch(), j->prevBatch(), j->totalRoomCountEstimate() };
};

template <>
struct QUOTIENT_API JsonObjectConverter<QueryPublicRoomsJob::Filter> {
    static void dumpTo(QJsonObject& jo, const QueryPublicRoomsJob::Filter& pod)
    {
        addParam<IfNotEmpty>(jo, "generic_search_term"_L1, pod.genericSearchTerm);
        addParam<IfNotEmpty>(jo, "room_types"_L1, pod.roomTypes);
    }
};

} // namespace Quotient

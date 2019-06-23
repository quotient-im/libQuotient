/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

#include "csapi/definitions/public_rooms_response.h"

#include "jobs/basejob.h"

namespace QMatrixClient
{

// Operations

/// Gets the visibility of a room in the directory
/*!
 * Gets the visibility of a given room on the server's public room directory.
 */
class GetRoomVisibilityOnDirectoryJob : public BaseJob
{
public:
    /*! Gets the visibility of a room in the directory
     * \param roomId
     *   The room ID.
     */
    explicit GetRoomVisibilityOnDirectoryJob(const QString& roomId);

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetRoomVisibilityOnDirectoryJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId);

    ~GetRoomVisibilityOnDirectoryJob() override;

    // Result properties

    /// The visibility of the room in the directory.
    const QString& visibility() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Sets the visibility of a room in the room directory
/*!
 * Sets the visibility of a given room in the server's public room
 * directory.
 *
 * Servers may choose to implement additional access control checks
 * here, for instance that room visibility can only be changed by
 * the room creator or a server administrator.
 */
class SetRoomVisibilityOnDirectoryJob : public BaseJob
{
public:
    /*! Sets the visibility of a room in the room directory
     * \param roomId
     *   The room ID.
     * \param visibility
     *   The new visibility setting for the room.
     *   Defaults to 'public'.
     */
    explicit SetRoomVisibilityOnDirectoryJob(const QString& roomId,
                                             const QString& visibility = {});
};

/// Lists the public rooms on the server.
/*!
 * Lists the public rooms on the server.
 *
 * This API returns paginated responses. The rooms are ordered by the number
 * of joined members, with the largest rooms first.
 */
class GetPublicRoomsJob : public BaseJob
{
public:
    /*! Lists the public rooms on the server.
     * \param limit
     *   Limit the number of results returned.
     * \param since
     *   A pagination token from a previous request, allowing clients to
     *   get the next (or previous) batch of rooms.
     *   The direction of pagination is specified solely by which token
     *   is supplied, rather than via an explicit flag.
     * \param server
     *   The server to fetch the public room lists from. Defaults to the
     *   local server.
     */
    explicit GetPublicRoomsJob(Omittable<int> limit = none,
                               const QString& since = {},
                               const QString& server = {});

    /*! Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetPublicRoomsJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, Omittable<int> limit = none,
                               const QString& since = {},
                               const QString& server = {});

    ~GetPublicRoomsJob() override;

    // Result properties

    /// A list of the rooms on the server.
    const PublicRoomsResponse& data() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

/// Lists the public rooms on the server with optional filter.
/*!
 * Lists the public rooms on the server, with optional filter.
 *
 * This API returns paginated responses. The rooms are ordered by the number
 * of joined members, with the largest rooms first.
 */
class QueryPublicRoomsJob : public BaseJob
{
public:
    // Inner data structures

    /// Filter to apply to the results.
    struct Filter
    {
        /// A string to search for in the room metadata, e.g. name,topic,
        /// canonical alias etc. (Optional).
        QString genericSearchTerm;
    };

    // Construction/destruction

    /*! Lists the public rooms on the server with optional filter.
     * \param server
     *   The server to fetch the public room lists from. Defaults to the
     *   local server.
     * \param limit
     *   Limit the number of results returned.
     * \param since
     *   A pagination token from a previous request, allowing clients
     *   to get the next (or previous) batch of rooms.  The direction
     *   of pagination is specified solely by which token is supplied,
     *   rather than via an explicit flag.
     * \param filter
     *   Filter to apply to the results.
     * \param includeAllNetworks
     *   Whether or not to include all known networks/protocols from
     *   application services on the homeserver. Defaults to false.
     * \param thirdPartyInstanceId
     *   The specific third party network/protocol to request from the
     *   homeserver. Can only be used if ``include_all_networks`` is false.
     */
    explicit QueryPublicRoomsJob(const QString& server = {},
                                 Omittable<int> limit = none,
                                 const QString& since = {},
                                 const Omittable<Filter>& filter = none,
                                 Omittable<bool> includeAllNetworks = none,
                                 const QString& thirdPartyInstanceId = {});

    ~QueryPublicRoomsJob() override;

    // Result properties

    /// A list of the rooms on the server.
    const PublicRoomsResponse& data() const;

protected:
    Status parseJson(const QJsonDocument& data) override;

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace QMatrixClient

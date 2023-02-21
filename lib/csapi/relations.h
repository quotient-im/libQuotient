/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../events/roomevent.h"
#include "../jobs/basejob.h"

namespace Quotient {

/*! \brief Get the child events for a given parent event.
 *
 * Retrieve all of the child events for a given parent event.
 *
 * Note that when paginating the `from` token should be "after" the `to` token
 * in terms of topological ordering, because it is only possible to paginate
 * "backwards" through events, starting at `from`.
 *
 * For example, passing a `from` token from page 2 of the results, and a `to`
 * token from page 1, would return the empty set. The caller can use a `from`
 * token from page 1 and a `to` token from page 2 to paginate over the same
 * range, however.
 */
class QUOTIENT_API GetRelatingEventsJob : public BaseJob {
public:
    /*! \brief Get the child events for a given parent event.
     *
     * \param roomId
     *   The ID of the room containing the parent event.
     *
     * \param eventId
     *   The ID of the parent event whose child events are to be returned.
     *
     * \param from
     *   The pagination token to start returning results from. If not supplied,
     * results start at the most recent topological event known to the server.
     *
     *   Can be a `next_batch` or `prev_batch` token from a previous call, or a
     * returned `start` token from
     * [`/messages`](/client-server-api/#get_matrixclientv3roomsroomidmessages),
     *   or a `next_batch` token from
     * [`/sync`](/client-server-api/#get_matrixclientv3sync).
     *
     * \param to
     *   The pagination token to stop returning results at. If not supplied,
     * results continue up to `limit` or until there are no more events.
     *
     *   Like `from`, this can be a previous token from a prior call to this
     * endpoint or from `/messages` or `/sync`.
     *
     * \param limit
     *   The maximum number of results to return in a single `chunk`. The server
     * can and should apply a maximum value to this parameter to avoid large
     * responses.
     *
     *   Similarly, the server should apply a default value when not supplied.
     *
     * \param dir
     *   Optional (default `b`) direction to return events from. If this is set
     * to `f`, events will be returned in chronological order starting at
     * `from`. If it is set to `b`, events will be returned in *reverse*
     * chronological order, again starting at `from`.
     */
    explicit GetRelatingEventsJob(const QString& roomId, const QString& eventId,
                                  const QString& from = {},
                                  const QString& to = {},
                                  Omittable<int> limit = none,
                                  const QString& dir = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRelatingEventsJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& eventId, const QString& from = {},
                               const QString& to = {},
                               Omittable<int> limit = none,
                               const QString& dir = {});

    // Result properties

    /// The child events of the requested event, ordered topologically
    /// most-recent first.
    RoomEvents chunk() { return takeFromJson<RoomEvents>("chunk"_ls); }

    /// An opaque string representing a pagination token. The absence of this
    /// token means there are no more results to fetch and the client should
    /// stop paginating.
    QString nextBatch() const { return loadFromJson<QString>("next_batch"_ls); }

    /// An opaque string representing a pagination token. The absence of this
    /// token means this is the start of the result set, i.e. this is the first
    /// batch/page.
    QString prevBatch() const { return loadFromJson<QString>("prev_batch"_ls); }
};

/*! \brief Get the child events for a given parent event, with a given
 * `relType`.
 *
 * Retrieve all of the child events for a given parent event which relate to the
 * parent using the given `relType`.
 *
 * Note that when paginating the `from` token should be "after" the `to` token
 * in terms of topological ordering, because it is only possible to paginate
 * "backwards" through events, starting at `from`.
 *
 * For example, passing a `from` token from page 2 of the results, and a `to`
 * token from page 1, would return the empty set. The caller can use a `from`
 * token from page 1 and a `to` token from page 2 to paginate over the same
 * range, however.
 */
class QUOTIENT_API GetRelatingEventsWithRelTypeJob : public BaseJob {
public:
    /*! \brief Get the child events for a given parent event, with a given
     * `relType`.
     *
     * \param roomId
     *   The ID of the room containing the parent event.
     *
     * \param eventId
     *   The ID of the parent event whose child events are to be returned.
     *
     * \param relType
     *   The [relationship type](/client-server-api/#relationship-types) to
     * search for.
     *
     * \param from
     *   The pagination token to start returning results from. If not supplied,
     * results start at the most recent topological event known to the server.
     *
     *   Can be a `next_batch` or `prev_batch` token from a previous call, or a
     * returned `start` token from
     * [`/messages`](/client-server-api/#get_matrixclientv3roomsroomidmessages),
     *   or a `next_batch` token from
     * [`/sync`](/client-server-api/#get_matrixclientv3sync).
     *
     * \param to
     *   The pagination token to stop returning results at. If not supplied,
     * results continue up to `limit` or until there are no more events.
     *
     *   Like `from`, this can be a previous token from a prior call to this
     * endpoint or from `/messages` or `/sync`.
     *
     * \param limit
     *   The maximum number of results to return in a single `chunk`. The server
     * can and should apply a maximum value to this parameter to avoid large
     * responses.
     *
     *   Similarly, the server should apply a default value when not supplied.
     *
     * \param dir
     *   Optional (default `b`) direction to return events from. If this is set
     * to `f`, events will be returned in chronological order starting at
     * `from`. If it is set to `b`, events will be returned in *reverse*
     * chronological order, again starting at `from`.
     */
    explicit GetRelatingEventsWithRelTypeJob(
        const QString& roomId, const QString& eventId, const QString& relType,
        const QString& from = {}, const QString& to = {},
        Omittable<int> limit = none, const QString& dir = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetRelatingEventsWithRelTypeJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& eventId, const QString& relType,
                               const QString& from = {}, const QString& to = {},
                               Omittable<int> limit = none,
                               const QString& dir = {});

    // Result properties

    /// The child events of the requested event, ordered topologically
    /// most-recent first. The events returned will match the `relType`
    /// supplied in the URL.
    RoomEvents chunk() { return takeFromJson<RoomEvents>("chunk"_ls); }

    /// An opaque string representing a pagination token. The absence of this
    /// token means there are no more results to fetch and the client should
    /// stop paginating.
    QString nextBatch() const { return loadFromJson<QString>("next_batch"_ls); }

    /// An opaque string representing a pagination token. The absence of this
    /// token means this is the start of the result set, i.e. this is the first
    /// batch/page.
    QString prevBatch() const { return loadFromJson<QString>("prev_batch"_ls); }
};

/*! \brief Get the child events for a given parent event, with a given `relType`
 * and `eventType`.
 *
 * Retrieve all of the child events for a given parent event which relate to the
 * parent using the given `relType` and have the given `eventType`.
 *
 * Note that when paginating the `from` token should be "after" the `to` token
 * in terms of topological ordering, because it is only possible to paginate
 * "backwards" through events, starting at `from`.
 *
 * For example, passing a `from` token from page 2 of the results, and a `to`
 * token from page 1, would return the empty set. The caller can use a `from`
 * token from page 1 and a `to` token from page 2 to paginate over the same
 * range, however.
 */
class QUOTIENT_API GetRelatingEventsWithRelTypeAndEventTypeJob
    : public BaseJob {
public:
    /*! \brief Get the child events for a given parent event, with a given
     * `relType` and `eventType`.
     *
     * \param roomId
     *   The ID of the room containing the parent event.
     *
     * \param eventId
     *   The ID of the parent event whose child events are to be returned.
     *
     * \param relType
     *   The [relationship type](/client-server-api/#relationship-types) to
     * search for.
     *
     * \param eventType
     *   The event type of child events to search for.
     *
     *   Note that in encrypted rooms this will typically always be
     * `m.room.encrypted` regardless of the event type contained within the
     * encrypted payload.
     *
     * \param from
     *   The pagination token to start returning results from. If not supplied,
     * results start at the most recent topological event known to the server.
     *
     *   Can be a `next_batch` or `prev_batch` token from a previous call, or a
     * returned `start` token from
     * [`/messages`](/client-server-api/#get_matrixclientv3roomsroomidmessages),
     *   or a `next_batch` token from
     * [`/sync`](/client-server-api/#get_matrixclientv3sync).
     *
     * \param to
     *   The pagination token to stop returning results at. If not supplied,
     * results continue up to `limit` or until there are no more events.
     *
     *   Like `from`, this can be a previous token from a prior call to this
     * endpoint or from `/messages` or `/sync`.
     *
     * \param limit
     *   The maximum number of results to return in a single `chunk`. The server
     * can and should apply a maximum value to this parameter to avoid large
     * responses.
     *
     *   Similarly, the server should apply a default value when not supplied.
     *
     * \param dir
     *   Optional (default `b`) direction to return events from. If this is set
     * to `f`, events will be returned in chronological order starting at
     * `from`. If it is set to `b`, events will be returned in *reverse*
     * chronological order, again starting at `from`.
     */
    explicit GetRelatingEventsWithRelTypeAndEventTypeJob(
        const QString& roomId, const QString& eventId, const QString& relType,
        const QString& eventType, const QString& from = {},
        const QString& to = {}, Omittable<int> limit = none,
        const QString& dir = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for
     * GetRelatingEventsWithRelTypeAndEventTypeJob is necessary but the job
     * itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               const QString& eventId, const QString& relType,
                               const QString& eventType,
                               const QString& from = {}, const QString& to = {},
                               Omittable<int> limit = none,
                               const QString& dir = {});

    // Result properties

    /// The child events of the requested event, ordered topologically
    /// most-recent first. The events returned will match the `relType` and
    /// `eventType` supplied in the URL.
    RoomEvents chunk() { return takeFromJson<RoomEvents>("chunk"_ls); }

    /// An opaque string representing a pagination token. The absence of this
    /// token means there are no more results to fetch and the client should
    /// stop paginating.
    QString nextBatch() const { return loadFromJson<QString>("next_batch"_ls); }

    /// An opaque string representing a pagination token. The absence of this
    /// token means this is the start of the result set, i.e. this is the first
    /// batch/page.
    QString prevBatch() const { return loadFromJson<QString>("prev_batch"_ls); }
};

} // namespace Quotient

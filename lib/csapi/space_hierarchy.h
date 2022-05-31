/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "csapi/../../event-schemas/schema/core-event-schema/stripped_state.h"
#include "csapi/definitions/public_rooms_chunk.h"

#include "jobs/basejob.h"

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
                                  Omittable<double> limit = none,
                                  Omittable<double> maxDepth = none,
                                  const QString& from = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetSpaceHierarchyJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomId,
                               Omittable<bool> suggestedOnly = none,
                               Omittable<double> limit = none,
                               Omittable<double> maxDepth = none,
                               const QString& from = {});

    // Result properties

    /// The rooms for the current page, with the current filters.
    QVector<QJsonObject> rooms() const
    {
        return loadFromJson<QVector<QJsonObject>>("rooms"_ls);
    }

    /// A token to supply to `from` to keep paginating the responses. Not
    /// present when there are no further results.
    QString nextBatch() const { return loadFromJson<QString>("next_batch"_ls); }
};

} // namespace Quotient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../jobs/basejob.h"

namespace Quotient {

/*! \brief Lists the user's current rooms.
 *
 * This API returns a list of the user's current rooms.
 */
class QUOTIENT_API GetJoinedRoomsJob : public BaseJob {
public:
    /// Lists the user's current rooms.
    explicit GetJoinedRoomsJob();

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetJoinedRoomsJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    /// The ID of each room in which the user has `joined` membership.
    QStringList joinedRooms() const
    {
        return loadFromJson<QStringList>("joined_rooms"_ls);
    }
};

} // namespace Quotient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../jobs/basejob.h"

namespace Quotient {

/*! \brief Set the position of the read marker for a room.
 *
 * Sets the position of the read marker for a given room, and optionally
 * the read receipt's location.
 */
class QUOTIENT_API SetReadMarkerJob : public BaseJob {
public:
    /*! \brief Set the position of the read marker for a room.
     *
     * \param roomId
     *   The room ID to set the read marker in for the user.
     *
     * \param mFullyRead
     *   The event ID the read marker should be located at. The
     *   event MUST belong to the room.
     *
     * \param mRead
     *   The event ID to set the read receipt location at. This is
     *   equivalent to calling `/receipt/m.read/$elsewhere:example.org`
     *   and is provided here to save that extra call.
     *
     * \param mReadPrivate
     *   The event ID to set the *private* read receipt location at. This
     *   equivalent to calling `/receipt/m.read.private/$elsewhere:example.org`
     *   and is provided here to save that extra call.
     */
    explicit SetReadMarkerJob(const QString& roomId,
                              const QString& mFullyRead = {},
                              const QString& mRead = {},
                              const QString& mReadPrivate = {});
};

} // namespace Quotient

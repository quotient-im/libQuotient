/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Informs the server that the user has started or stopped typing.
 *
 * This tells the server that the user is typing for the next N
 * milliseconds where N is the value specified in the `timeout` key.
 * Alternatively, if `typing` is `false`, it tells the server that the
 * user has stopped typing.
 */
class QUOTIENT_API SetTypingJob : public BaseJob {
public:
    /*! \brief Informs the server that the user has started or stopped typing.
     *
     * \param userId
     *   The user who has started to type.
     *
     * \param roomId
     *   The room in which the user is typing.
     *
     * \param typing
     *   Whether the user is typing or not. If `false`, the `timeout`
     *   key can be omitted.
     *
     * \param timeout
     *   The length of time in milliseconds to mark this user as typing.
     */
    explicit SetTypingJob(const QString& userId, const QString& roomId,
                          bool typing, Omittable<int> timeout = none);
};

} // namespace Quotient

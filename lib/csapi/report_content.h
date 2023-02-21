/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../jobs/basejob.h"

namespace Quotient {

/*! \brief Reports an event as inappropriate.
 *
 * Reports an event as inappropriate to the server, which may then notify
 * the appropriate people.
 */
class QUOTIENT_API ReportContentJob : public BaseJob {
public:
    /*! \brief Reports an event as inappropriate.
     *
     * \param roomId
     *   The room in which the event being reported is located.
     *
     * \param eventId
     *   The event to report.
     *
     * \param score
     *   The score to rate this content as where -100 is most offensive
     *   and 0 is inoffensive.
     *
     * \param reason
     *   The reason the content is being reported. May be blank.
     */
    explicit ReportContentJob(const QString& roomId, const QString& eventId,
                              Omittable<int> score = none,
                              const QString& reason = {});
};

} // namespace Quotient

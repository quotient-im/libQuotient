/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

/*! \brief Ask the homeserver to ping the application service to ensure the
 * connection works.
 *
 * This API asks the homeserver to call the
 * [`/_matrix/app/v1/ping`](#post_matrixappv1ping) endpoint on the
 * application service to ensure that the homeserver can communicate
 * with the application service.
 *
 * This API requires the use of an application service access token (`as_token`)
 * instead of a typical client's access token. This API cannot be invoked by
 * users who are not identified as application services. Additionally, the
 * appservice ID in the path must be the same as the appservice whose `as_token`
 * is being used.
 */
class QUOTIENT_API PingAppserviceJob : public BaseJob {
public:
    /*! \brief Ask the homeserver to ping the application service to ensure the
     * connection works.
     *
     * \param appserviceId
     *   The appservice ID of the appservice to ping. This must be the same
     *   as the appservice whose `as_token` is being used to authenticate the
     *   request.
     *
     * \param transactionId
     *   An optional transaction ID that is passed through to the
     * `/_matrix/app/v1/ping` call.
     */
    explicit PingAppserviceJob(const QString& appserviceId,
                               const QString& transactionId = {});

    // Result properties

    /// The duration in milliseconds that the
    /// [`/_matrix/app/v1/ping`](#post_matrixappv1ping)
    /// request took from the homeserver's point of view.
    int durationMs() const { return loadFromJson<int>("duration_ms"_ls); }
};

} // namespace Quotient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Query if a given registration token is still valid.
 *
 * Queries the server to determine if a given registration token is still
 * valid at the time of request. This is a point-in-time check where the
 * token might still expire by the time it is used.
 *
 * Servers should be sure to rate limit this endpoint to avoid brute force
 * attacks.
 */
class QUOTIENT_API RegistrationTokenValidityJob : public BaseJob {
public:
    /*! \brief Query if a given registration token is still valid.
     *
     * \param token
     *   The token to check validity of.
     */
    explicit RegistrationTokenValidityJob(const QString& token);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for RegistrationTokenValidityJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& token);

    // Result properties

    /// True if the token is still valid, false otherwise. This should
    /// additionally be false if the token is not a recognised token by
    /// the server.
    bool valid() const { return loadFromJson<bool>("valid"_ls); }
};

} // namespace Quotient

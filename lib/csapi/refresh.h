/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../jobs/basejob.h"

namespace Quotient {

/*! \brief Refresh an access token
 *
 * Refresh an access token. Clients should use the returned access token
 * when making subsequent API calls, and store the returned refresh token
 * (if given) in order to refresh the new access token when necessary.
 *
 * After an access token has been refreshed, a server can choose to
 * invalidate the old access token immediately, or can choose not to, for
 * example if the access token would expire soon anyways. Clients should
 * not make any assumptions about the old access token still being valid,
 * and should use the newly provided access token instead.
 *
 * The old refresh token remains valid until the new access token or refresh
 * token is used, at which point the old refresh token is revoked.
 *
 * Note that this endpoint does not require authentication via an
 * access token. Authentication is provided via the refresh token.
 *
 * Application Service identity assertion is disabled for this endpoint.
 */
class QUOTIENT_API RefreshJob : public BaseJob {
public:
    /*! \brief Refresh an access token
     *
     * \param refreshToken
     *   The refresh token
     */
    explicit RefreshJob(const QString& refreshToken);

    // Result properties

    /// The new access token to use.
    QString accessToken() const
    {
        return loadFromJson<QString>("access_token"_ls);
    }

    /// The new refresh token to use when the access token needs to
    /// be refreshed again. If not given, the old refresh token can
    /// be re-used.
    QString refreshToken() const
    {
        return loadFromJson<QString>("refresh_token"_ls);
    }

    /// The lifetime of the access token, in milliseconds. If not
    /// given, the client can assume that the access token will not
    /// expire.
    Omittable<int> expiresInMs() const
    {
        return loadFromJson<Omittable<int>>("expires_in_ms"_ls);
    }
};

} // namespace Quotient

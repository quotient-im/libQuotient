/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Redirect the user's browser to the SSO interface.
 *
 * A web-based Matrix client should instruct the user's browser to
 * navigate to this endpoint in order to log in via SSO.
 *
 * The server MUST respond with an HTTP redirect to the SSO interface,
 * or present a page which lets the user select an IdP to continue
 * with in the event multiple are supported by the server.
 */
class RedirectToSSOJob : public BaseJob {
public:
    /*! \brief Redirect the user's browser to the SSO interface.
     *
     * \param redirectUrl
     *   URI to which the user will be redirected after the homeserver has
     *   authenticated the user with SSO.
     */
    explicit RedirectToSSOJob(const QString& redirectUrl);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for RedirectToSSOJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& redirectUrl);
};

/*! \brief Redirect the user's browser to the SSO interface for an IdP.
 *
 * This endpoint is the same as `/login/sso/redirect`, though with an
 * IdP ID from the original `identity_providers` array to inform the
 * server of which IdP the client/user would like to continue with.
 *
 * The server MUST respond with an HTTP redirect to the SSO interface
 * for that IdP.
 */
class RedirectToIdPJob : public BaseJob {
public:
    /*! \brief Redirect the user's browser to the SSO interface for an IdP.
     *
     * \param idpId
     *   The `id` of the IdP from the `m.login.sso` `identity_providers`
     *   array denoting the user's selection.
     *
     * \param redirectUrl
     *   URI to which the user will be redirected after the homeserver has
     *   authenticated the user with SSO.
     */
    explicit RedirectToIdPJob(const QString& idpId, const QString& redirectUrl);

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for RedirectToIdPJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& idpId,
                               const QString& redirectUrl);
};

} // namespace Quotient

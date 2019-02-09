/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    /// Redirect the user's browser to the SSO interface.
    ///
    /// A web-based Matrix client should instruct the user's browser to
    /// navigate to this endpoint in order to log in via SSO.
    /// 
    /// The server MUST respond with an HTTP redirect to the SSO interface.
    class RedirectToSSOJob : public BaseJob
    {
        public:
            /*! Redirect the user's browser to the SSO interface.
             * \param redirectUrl
             *   URI to which the user will be redirected after the homeserver has
             *   authenticated the user with SSO.
             */
            explicit RedirectToSSOJob(const QString& redirectUrl);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * RedirectToSSOJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& redirectUrl);

    };
} // namespace QMatrixClient

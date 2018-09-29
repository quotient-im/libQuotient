/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    /// Invalidates a user access token
    ///
    /// Invalidates an existing access token, so that it can no longer be used for
    /// authorization.
    class LogoutJob : public BaseJob
    {
        public:
            explicit LogoutJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * LogoutJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

    };

    /// Invalidates all access tokens for a user
    ///
    /// Invalidates all access tokens for a user, so that they can no longer be used for
    /// authorization. This includes the access token that made this request. 
    /// 
    /// This endpoint does not require UI authorization because UI authorization is
    /// designed to protect against attacks where the someone gets hold of a single access
    /// token then takes over the account. This endpoint invalidates all access tokens for
    /// the user, including the token used in the request, and therefore the attacker is
    /// unable to take over the account in this way.
    class LogoutAllJob : public BaseJob
    {
        public:
            explicit LogoutAllJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * LogoutAllJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

    };
} // namespace QMatrixClient

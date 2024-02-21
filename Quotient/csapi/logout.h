// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Invalidates a user access token
//!
//! Invalidates an existing access token, so that it can no longer be used for
//! authorization. The device associated with the access token is also deleted.
//! [Device keys](/client-server-api/#device-keys) for the device are deleted alongside the device.
class QUOTIENT_API LogoutJob : public BaseJob {
public:
    explicit LogoutJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for LogoutJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl);
};

//! \brief Invalidates all access tokens for a user
//!
//! Invalidates all access tokens for a user, so that they can no longer be used for
//! authorization. This includes the access token that made this request. All devices
//! for the user are also deleted. [Device keys](/client-server-api/#device-keys) for the device are
//! deleted alongside the device.
//!
//! This endpoint does not use the [User-Interactive Authentication
//! API](/client-server-api/#user-interactive-authentication-api) because User-Interactive
//! Authentication is designed to protect against attacks where the someone gets hold of a single
//! access token then takes over the account. This endpoint invalidates all access tokens for the
//! user, including the token used in the request, and therefore the attacker is unable to take over
//! the account in this way.
class QUOTIENT_API LogoutAllJob : public BaseJob {
public:
    explicit LogoutAllJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for LogoutAllJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl);
};

} // namespace Quotient

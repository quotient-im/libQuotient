// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/csapi/definitions/auth_data.h>

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Optional endpoint to generate a single-use, time-limited, `m.login.token` token.
//!
//! Optional endpoint - the server is not required to implement this endpoint if it does not
//! intend to use or support this functionality.
//!
//! This API endpoint uses the [User-Interactive Authentication
//! API](/client-server-api/#user-interactive-authentication-api).
//!
//! An already-authenticated client can call this endpoint to generate a single-use, time-limited,
//! token for an unauthenticated client to log in with, becoming logged in as the same user which
//! called this endpoint. The unauthenticated client uses the generated token in a `m.login.token`
//! login flow with the homeserver.
//!
//! Clients, both authenticated and unauthenticated, might wish to hide user interface which exposes
//! this feature if the server is not offering it. Authenticated clients can check for support on
//! a per-user basis with the `m.get_login_token`
//! [capability](/client-server-api/#capabilities-negotiation), while unauthenticated clients can
//! detect server support by looking for an `m.login.token` login flow with `get_login_token: true`
//! on [`GET /login`](/client-server-api/#post_matrixclientv3login).
//!
//! In v1.7 of the specification, transmission of the generated token to an unauthenticated client
//! is left as an implementation detail. Future MSCs such as
//! [MSC3906](https://github.com/matrix-org/matrix-spec-proposals/pull/3906) might standardise a way
//! to transmit the token between clients.
//!
//! The generated token MUST only be valid for a single login, enforced by the server. Clients which
//! intend to log in multiple devices must generate a token for each.
//!
//! With other User-Interactive Authentication (UIA)-supporting endpoints, servers sometimes do not
//! re-prompt for verification if the session recently passed UIA. For this endpoint, servers MUST
//! always re-prompt the user for verification to ensure explicit consent is gained for each
//! additional client.
//!
//! Servers are encouraged to apply stricter than normal rate limiting to this endpoint, such as
//! maximum of 1 request per minute.
class QUOTIENT_API GenerateLoginTokenJob : public BaseJob {
public:
    //! \param auth
    //!   Additional authentication information for the user-interactive authentication API.
    explicit GenerateLoginTokenJob(const std::optional<AuthenticationData>& auth = std::nullopt);

    // Result properties

    //! The login token for the `m.login.token` login flow.
    QString loginToken() const { return loadFromJson<QString>("login_token"_L1); }

    //! The time remaining in milliseconds until the homeserver will no longer accept the token.
    //! `120000` (2 minutes) is recommended as a default.
    int expiresInMs() const { return loadFromJson<int>("expires_in_ms"_L1); }

    struct Response {
        //! The login token for the `m.login.token` login flow.
        QString loginToken{};

        //! The time remaining in milliseconds until the homeserver will no longer accept the token.
        //! `120000` (2 minutes) is recommended as a default.
        int expiresInMs{};
    };
};

template <std::derived_from<GenerateLoginTokenJob> JobT>
constexpr inline auto doCollectResponse<JobT> = [](JobT* j) -> GenerateLoginTokenJob::Response {
    return { j->loginToken(), j->expiresInMs() };
};

} // namespace Quotient

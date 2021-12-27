/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "csapi/definitions/openid_token.h"

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Get an OpenID token object to verify the requester's identity.
 *
 * Gets an OpenID token object that the requester may supply to another
 * service to verify their identity in Matrix. The generated token is only
 * valid for exchanging for user information from the federation API for
 * OpenID.
 *
 * The access token generated is only valid for the OpenID API. It cannot
 * be used to request another OpenID access token or call `/sync`, for
 * example.
 */
class RequestOpenIdTokenJob : public BaseJob {
public:
    /*! \brief Get an OpenID token object to verify the requester's identity.
     *
     * \param userId
     *   The user to request and OpenID token for. Should be the user who
     *   is authenticated for the request.
     *
     * \param body
     *   An empty object. Reserved for future expansion.
     */
    explicit RequestOpenIdTokenJob(const QString& userId,
                                   const QJsonObject& body = {});

    // Result properties

    /// OpenID token information. This response is nearly compatible with the
    /// response documented in the
    /// [OpenID Connect 1.0
    /// Specification](http://openid.net/specs/openid-connect-core-1_0.html#TokenResponse)
    /// with the only difference being the lack of an `id_token`. Instead,
    /// the Matrix homeserver's name is provided.
    OpenidToken tokenData() const { return fromJson<OpenidToken>(jsonData()); }
};

} // namespace Quotient

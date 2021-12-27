/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Gets information about the owner of an access token.
 *
 * Gets information about the owner of a given access token.
 *
 * Note that, as with the rest of the Client-Server API,
 * Application Services may masquerade as users within their
 * namespace by giving a `user_id` query parameter. In this
 * situation, the server should verify that the given `user_id`
 * is registered by the appservice, and return it in the response
 * body.
 */
class GetTokenOwnerJob : public BaseJob {
public:
    /// Gets information about the owner of an access token.
    explicit GetTokenOwnerJob();

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetTokenOwnerJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    /// The user ID that owns the access token.
    QString userId() const { return loadFromJson<QString>("user_id"_ls); }

    /// Device ID associated with the access token. If no device
    /// is associated with the access token (such as in the case
    /// of application services) then this field can be omitted.
    /// Otherwise this is required.
    QString deviceId() const { return loadFromJson<QString>("device_id"_ls); }
};

} // namespace Quotient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/csapi/definitions/auth_data.h>
#include <Quotient/csapi/definitions/cross_signing_key.h>
#include <Quotient/jobs/basejob.h>

namespace Quotient {

/*! \brief Upload cross-signing keys.
 *
 * Publishes cross-signing keys for the user.
 *
 * This API endpoint uses the [User-Interactive Authentication
 * API](/client-server-api/#user-interactive-authentication-api).
 */
class QUOTIENT_API UploadCrossSigningKeysJob : public BaseJob {
public:
    /*! \brief Upload cross-signing keys.
     *
     * \param masterKey
     *   Optional. The user\'s master key.
     *
     * \param selfSigningKey
     *   Optional. The user\'s self-signing key. Must be signed by
     *   the accompanying master key, or by the user\'s most recently
     *   uploaded master key if no master key is included in the
     *   request.
     *
     * \param userSigningKey
     *   Optional. The user\'s user-signing key. Must be signed by
     *   the accompanying master key, or by the user\'s most recently
     *   uploaded master key if no master key is included in the
     *   request.
     *
     * \param auth
     *   Additional authentication information for the
     *   user-interactive authentication API.
     */
    explicit UploadCrossSigningKeysJob(
        const Omittable<CrossSigningKey>& masterKey = none,
        const Omittable<CrossSigningKey>& selfSigningKey = none,
        const Omittable<CrossSigningKey>& userSigningKey = none,
        const Omittable<AuthenticationData>& auth = none);
};

/*! \brief Upload cross-signing signatures.
 *
 * Publishes cross-signing signatures for the user.
 *
 * The request body is a map from user ID to key ID to signed JSON object.
 * The signed JSON object must match the key previously uploaded or
 * retrieved for the given key ID, with the exception of the `signatures`
 * property, which contains the new signature(s) to add.
 */
class QUOTIENT_API UploadCrossSigningSignaturesJob : public BaseJob {
public:
    /*! \brief Upload cross-signing signatures.
     *
     * \param signatures
     *   A map from user ID to key ID to signed JSON objects containing the
     *   signatures to be published.
     */
    explicit UploadCrossSigningSignaturesJob(
        const QHash<QString, QHash<QString, QJsonObject>>& signatures);

    // Result properties

    /// A map from user ID to key ID to an error for any signatures
    /// that failed.  If a signature was invalid, the `errcode` will
    /// be set to `M_INVALID_SIGNATURE`.
    QHash<QString, QHash<QString, QJsonObject>> failures() const
    {
        return loadFromJson<QHash<QString, QHash<QString, QJsonObject>>>(
            "failures"_ls);
    }
};

} // namespace Quotient

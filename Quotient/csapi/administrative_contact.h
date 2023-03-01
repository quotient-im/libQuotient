/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "csapi/definitions/auth_data.h"
#include "csapi/definitions/request_email_validation.h"
#include "csapi/definitions/request_msisdn_validation.h"
#include "csapi/definitions/request_token_response.h"

#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Gets a list of a user's third party identifiers.
 *
 * Gets a list of the third party identifiers that the homeserver has
 * associated with the user's account.
 *
 * This is *not* the same as the list of third party identifiers bound to
 * the user's Matrix ID in identity servers.
 *
 * Identifiers in this list may be used by the homeserver as, for example,
 * identifiers that it will accept to reset the user's account password.
 */
class QUOTIENT_API GetAccount3PIDsJob : public BaseJob {
public:
    // Inner data structures

    /// Gets a list of the third party identifiers that the homeserver has
    /// associated with the user's account.
    ///
    /// This is *not* the same as the list of third party identifiers bound to
    /// the user's Matrix ID in identity servers.
    ///
    /// Identifiers in this list may be used by the homeserver as, for example,
    /// identifiers that it will accept to reset the user's account password.
    struct ThirdPartyIdentifier {
        /// The medium of the third party identifier.
        QString medium;
        /// The third party identifier address.
        QString address;
        /// The timestamp, in milliseconds, when the identifier was
        /// validated by the identity server.
        qint64 validatedAt;
        /// The timestamp, in milliseconds, when the homeserver associated the
        /// third party identifier with the user.
        qint64 addedAt;
    };

    // Construction/destruction

    /// Gets a list of a user's third party identifiers.
    explicit GetAccount3PIDsJob();

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetAccount3PIDsJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    /// Gets a list of the third party identifiers that the homeserver has
    /// associated with the user's account.
    ///
    /// This is *not* the same as the list of third party identifiers bound to
    /// the user's Matrix ID in identity servers.
    ///
    /// Identifiers in this list may be used by the homeserver as, for example,
    /// identifiers that it will accept to reset the user's account password.
    QVector<ThirdPartyIdentifier> threepids() const
    {
        return loadFromJson<QVector<ThirdPartyIdentifier>>("threepids"_ls);
    }
};

template <>
struct JsonObjectConverter<GetAccount3PIDsJob::ThirdPartyIdentifier> {
    static void fillFrom(const QJsonObject& jo,
                         GetAccount3PIDsJob::ThirdPartyIdentifier& result)
    {
        fromJson(jo.value("medium"_ls), result.medium);
        fromJson(jo.value("address"_ls), result.address);
        fromJson(jo.value("validated_at"_ls), result.validatedAt);
        fromJson(jo.value("added_at"_ls), result.addedAt);
    }
};

/*! \brief Adds contact information to the user's account.
 *
 * Adds contact information to the user's account.
 *
 * This endpoint is deprecated in favour of the more specific `/3pid/add`
 * and `/3pid/bind` endpoints.
 *
 * **Note:**
 * Previously this endpoint supported a `bind` parameter. This parameter
 * has been removed, making this endpoint behave as though it was `false`.
 * This results in this endpoint being an equivalent to `/3pid/bind` rather
 * than dual-purpose.
 */
class QUOTIENT_API Post3PIDsJob : public BaseJob {
public:
    // Inner data structures

    /// The third party credentials to associate with the account.
    struct ThreePidCredentials {
        /// The client secret used in the session with the identity server.
        QString clientSecret;
        /// The identity server to use.
        QString idServer;
        /// An access token previously registered with the identity server.
        /// Servers can treat this as optional to distinguish between
        /// r0.5-compatible clients and this specification version.
        QString idAccessToken;
        /// The session identifier given by the identity server.
        QString sid;
    };

    // Construction/destruction

    /*! \brief Adds contact information to the user's account.
     *
     * \param threePidCreds
     *   The third party credentials to associate with the account.
     */
    explicit Post3PIDsJob(const ThreePidCredentials& threePidCreds);

    // Result properties

    /// An optional field containing a URL where the client must
    /// submit the validation token to, with identical parameters
    /// to the Identity Service API's `POST
    /// /validate/email/submitToken` endpoint (without the requirement
    /// for an access token). The homeserver must send this token to the
    /// user (if applicable), who should then be prompted to provide it
    /// to the client.
    ///
    /// If this field is not present, the client can assume that
    /// verification will happen without the client's involvement
    /// provided the homeserver advertises this specification version
    /// in the `/versions` response (ie: r0.5.0).
    QUrl submitUrl() const { return loadFromJson<QUrl>("submit_url"_ls); }
};

template <>
struct JsonObjectConverter<Post3PIDsJob::ThreePidCredentials> {
    static void dumpTo(QJsonObject& jo,
                       const Post3PIDsJob::ThreePidCredentials& pod)
    {
        addParam<>(jo, QStringLiteral("client_secret"), pod.clientSecret);
        addParam<>(jo, QStringLiteral("id_server"), pod.idServer);
        addParam<>(jo, QStringLiteral("id_access_token"), pod.idAccessToken);
        addParam<>(jo, QStringLiteral("sid"), pod.sid);
    }
};

/*! \brief Adds contact information to the user's account.
 *
 * This API endpoint uses the [User-Interactive Authentication
 * API](/client-server-api/#user-interactive-authentication-api).
 *
 * Adds contact information to the user's account. Homeservers should use 3PIDs
 * added through this endpoint for password resets instead of relying on the
 * identity server.
 *
 * Homeservers should prevent the caller from adding a 3PID to their account if
 * it has already been added to another user's account on the homeserver.
 */
class QUOTIENT_API Add3PIDJob : public BaseJob {
public:
    /*! \brief Adds contact information to the user's account.
     *
     * \param clientSecret
     *   The client secret used in the session with the homeserver.
     *
     * \param sid
     *   The session identifier given by the homeserver.
     *
     * \param auth
     *   Additional authentication information for the
     *   user-interactive authentication API.
     */
    explicit Add3PIDJob(const QString& clientSecret, const QString& sid,
                        const Omittable<AuthenticationData>& auth = none);
};

/*! \brief Binds a 3PID to the user's account through an Identity Service.
 *
 * Binds a 3PID to the user's account through the specified identity server.
 *
 * Homeservers should not prevent this request from succeeding if another user
 * has bound the 3PID. Homeservers should simply proxy any errors received by
 * the identity server to the caller.
 *
 * Homeservers should track successful binds so they can be unbound later.
 */
class QUOTIENT_API Bind3PIDJob : public BaseJob {
public:
    /*! \brief Binds a 3PID to the user's account through an Identity Service.
     *
     * \param clientSecret
     *   The client secret used in the session with the identity server.
     *
     * \param idServer
     *   The identity server to use.
     *
     * \param idAccessToken
     *   An access token previously registered with the identity server.
     *
     * \param sid
     *   The session identifier given by the identity server.
     */
    explicit Bind3PIDJob(const QString& clientSecret, const QString& idServer,
                         const QString& idAccessToken, const QString& sid);
};

/*! \brief Deletes a third party identifier from the user's account
 *
 * Removes a third party identifier from the user's account. This might not
 * cause an unbind of the identifier from the identity server.
 *
 * Unlike other endpoints, this endpoint does not take an `id_access_token`
 * parameter because the homeserver is expected to sign the request to the
 * identity server instead.
 */
class QUOTIENT_API Delete3pidFromAccountJob : public BaseJob {
public:
    /*! \brief Deletes a third party identifier from the user's account
     *
     * \param medium
     *   The medium of the third party identifier being removed.
     *
     * \param address
     *   The third party address being removed.
     *
     * \param idServer
     *   The identity server to unbind from. If not provided, the homeserver
     *   MUST use the `id_server` the identifier was added through. If the
     *   homeserver does not know the original `id_server`, it MUST return
     *   a `id_server_unbind_result` of `no-support`.
     */
    explicit Delete3pidFromAccountJob(const QString& medium,
                                      const QString& address,
                                      const QString& idServer = {});

    // Result properties

    /// An indicator as to whether or not the homeserver was able to unbind
    /// the 3PID from the identity server. `success` indicates that the
    /// identity server has unbound the identifier whereas `no-support`
    /// indicates that the identity server refuses to support the request
    /// or the homeserver was not able to determine an identity server to
    /// unbind from.
    QString idServerUnbindResult() const
    {
        return loadFromJson<QString>("id_server_unbind_result"_ls);
    }
};

/*! \brief Removes a user's third party identifier from an identity server.
 *
 * Removes a user's third party identifier from the provided identity server
 * without removing it from the homeserver.
 *
 * Unlike other endpoints, this endpoint does not take an `id_access_token`
 * parameter because the homeserver is expected to sign the request to the
 * identity server instead.
 */
class QUOTIENT_API Unbind3pidFromAccountJob : public BaseJob {
public:
    /*! \brief Removes a user's third party identifier from an identity server.
     *
     * \param medium
     *   The medium of the third party identifier being removed.
     *
     * \param address
     *   The third party address being removed.
     *
     * \param idServer
     *   The identity server to unbind from. If not provided, the homeserver
     *   MUST use the `id_server` the identifier was added through. If the
     *   homeserver does not know the original `id_server`, it MUST return
     *   a `id_server_unbind_result` of `no-support`.
     */
    explicit Unbind3pidFromAccountJob(const QString& medium,
                                      const QString& address,
                                      const QString& idServer = {});

    // Result properties

    /// An indicator as to whether or not the identity server was able to unbind
    /// the 3PID. `success` indicates that the identity server has unbound the
    /// identifier whereas `no-support` indicates that the identity server
    /// refuses to support the request or the homeserver was not able to
    /// determine an identity server to unbind from.
    QString idServerUnbindResult() const
    {
        return loadFromJson<QString>("id_server_unbind_result"_ls);
    }
};

/*! \brief Begins the validation process for an email address for association
 * with the user's account.
 *
 * The homeserver must check that the given email address is **not**
 * already associated with an account on this homeserver. This API should
 * be used to request validation tokens when adding an email address to an
 * account. This API's parameters and response are identical to that of
 * the
 * [`/register/email/requestToken`](/client-server-api/#post_matrixclientv3registeremailrequesttoken)
 * endpoint. The homeserver should validate
 * the email itself, either by sending a validation email itself or by using
 * a service it has control over.
 */
class QUOTIENT_API RequestTokenTo3PIDEmailJob : public BaseJob {
public:
    /*! \brief Begins the validation process for an email address for
     * association with the user's account.
     *
     * \param body
     *   The homeserver must check that the given email address is **not**
     *   already associated with an account on this homeserver. This API should
     *   be used to request validation tokens when adding an email address to an
     *   account. This API's parameters and response are identical to that of
     *   the
     * [`/register/email/requestToken`](/client-server-api/#post_matrixclientv3registeremailrequesttoken)
     *   endpoint. The homeserver should validate
     *   the email itself, either by sending a validation email itself or by
     * using a service it has control over.
     */
    explicit RequestTokenTo3PIDEmailJob(const EmailValidationData& body);

    // Result properties

    /// An email was sent to the given address. Note that this may be an
    /// email containing the validation token or it may be informing the
    /// user of an error.
    RequestTokenResponse response() const
    {
        return fromJson<RequestTokenResponse>(jsonData());
    }
};

/*! \brief Begins the validation process for a phone number for association with
 * the user's account.
 *
 * The homeserver must check that the given phone number is **not**
 * already associated with an account on this homeserver. This API should
 * be used to request validation tokens when adding a phone number to an
 * account. This API's parameters and response are identical to that of
 * the
 * [`/register/msisdn/requestToken`](/client-server-api/#post_matrixclientv3registermsisdnrequesttoken)
 * endpoint. The homeserver should validate
 * the phone number itself, either by sending a validation message itself or by
 * using a service it has control over.
 */
class QUOTIENT_API RequestTokenTo3PIDMSISDNJob : public BaseJob {
public:
    /*! \brief Begins the validation process for a phone number for association
     * with the user's account.
     *
     * \param body
     *   The homeserver must check that the given phone number is **not**
     *   already associated with an account on this homeserver. This API should
     *   be used to request validation tokens when adding a phone number to an
     *   account. This API's parameters and response are identical to that of
     *   the
     * [`/register/msisdn/requestToken`](/client-server-api/#post_matrixclientv3registermsisdnrequesttoken)
     *   endpoint. The homeserver should validate
     *   the phone number itself, either by sending a validation message itself
     * or by using a service it has control over.
     */
    explicit RequestTokenTo3PIDMSISDNJob(const MsisdnValidationData& body);

    // Result properties

    /// An SMS message was sent to the given phone number.
    RequestTokenResponse response() const
    {
        return fromJson<RequestTokenResponse>(jsonData());
    }
};

} // namespace Quotient

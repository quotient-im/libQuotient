/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include "csapi/../identity/definitions/sid.h"
#include <QtCore/QVector>

namespace QMatrixClient {
    // Operations

    /// Gets a list of a user's third party identifiers.
    ///
    /// Gets a list of the third party identifiers that the homeserver has
    /// associated with the user's account.
    ///
    /// This is *not* the same as the list of third party identifiers bound to
    /// the user's Matrix ID in identity servers.
    ///
    /// Identifiers in this list may be used by the homeserver as, for example,
    /// identifiers that it will accept to reset the user's account password.
    class GetAccount3PIDsJob : public BaseJob
    {
        public:
        // Inner data structures

        /// Gets a list of the third party identifiers that the homeserver has
        /// associated with the user's account.
        ///
        /// This is *not* the same as the list of third party identifiers bound
        /// to the user's Matrix ID in identity servers.
        ///
        /// Identifiers in this list may be used by the homeserver as, for
        /// example, identifiers that it will accept to reset the user's account
        /// password.
        struct ThirdPartyIdentifier {
            /// The medium of the third party identifier.
            QString medium;
            /// The third party identifier address.
            QString address;
            /// The timestamp, in milliseconds, when the identifier was
            /// validated by the identity server.
            qint64 validatedAt;
            /// The timestamp, in milliseconds, when the homeserver associated
            /// the third party identifier with the user.
            qint64 addedAt;
        };

        // Construction/destruction

        explicit GetAccount3PIDsJob();

        /*! Construct a URL without creating a full-fledged job object
         *
         * This function can be used when a URL for
         * GetAccount3PIDsJob is necessary but the job
         * itself isn't.
         */
        static QUrl makeRequestUrl(QUrl baseUrl);

        ~GetAccount3PIDsJob() override;

        // Result properties

        /// Gets a list of the third party identifiers that the homeserver has
        /// associated with the user's account.
        ///
        /// This is *not* the same as the list of third party identifiers bound
        /// to the user's Matrix ID in identity servers.
        ///
        /// Identifiers in this list may be used by the homeserver as, for
        /// example, identifiers that it will accept to reset the user's account
        /// password.
        const QVector<ThirdPartyIdentifier>& threepids() const;

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };

    /// Adds contact information to the user's account.
    ///
    /// Adds contact information to the user's account.
    class Post3PIDsJob : public BaseJob
    {
        public:
        // Inner data structures

        /// The third party credentials to associate with the account.
        struct ThreePidCredentials {
            /// The client secret used in the session with the identity server.
            QString clientSecret;
            /// The identity server to use.
            QString idServer;
            /// The session identifier given by the identity server.
            QString sid;
        };

        // Construction/destruction

        /*! Adds contact information to the user's account.
         * \param threePidCreds
         *   The third party credentials to associate with the account.
         * \param bind
         *   Whether the homeserver should also bind this third party
         *   identifier to the account's Matrix ID with the passed identity
         *   server. Default: ``false``.
         */
        explicit Post3PIDsJob(const ThreePidCredentials& threePidCreds,
                              Omittable<bool> bind = none);
    };

    /// Deletes a third party identifier from the user's account
    ///
    /// Removes a third party identifier from the user's account. This might not
    /// cause an unbind of the identifier from the identity server.
    class Delete3pidFromAccountJob : public BaseJob
    {
        public:
        /*! Deletes a third party identifier from the user's account
         * \param medium
         *   The medium of the third party identifier being removed.
         * \param address
         *   The third party address being removed.
         */
        explicit Delete3pidFromAccountJob(const QString& medium,
                                          const QString& address);
    };

    /// Begins the validation process for an email address for association with
    /// the user's account.
    ///
    /// Proxies the Identity Service API ``validate/email/requestToken``, but
    /// first checks that the given email address is **not** already associated
    /// with an account on this homeserver. This API should be used to request
    /// validation tokens when adding an email address to an account. This API's
    /// parameters and response are identical to that of the
    /// |/register/email/requestToken|_ endpoint.
    class RequestTokenTo3PIDEmailJob : public BaseJob
    {
        public:
        /*! Begins the validation process for an email address for association with the user's account.
         * \param clientSecret
         *   A unique string generated by the client, and used to identify the
         *   validation attempt. It must be a string consisting of the
         * characters
         *   ``[0-9a-zA-Z.=_-]``. Its length must not exceed 255 characters and
         * it must not be empty. \param email The email address to validate.
         * \param sendAttempt
         *   The server will only send an email if the ``send_attempt``
         *   is a number greater than the most recent one which it has seen,
         *   scoped to that ``email`` + ``client_secret`` pair. This is to
         *   avoid repeatedly sending the same email in the case of request
         *   retries between the POSTing user and the identity server.
         *   The client should increment this value if they desire a new
         *   email (e.g. a reminder) to be sent.
         * \param idServer
         *   The hostname of the identity server to communicate with. May
         *   optionally include a port.
         * \param nextLink
         *   Optional. When the validation is completed, the identity
         *   server will redirect the user to this URL.
         */
        explicit RequestTokenTo3PIDEmailJob(const QString& clientSecret,
                                            const QString& email,
                                            int sendAttempt,
                                            const QString& idServer,
                                            const QString& nextLink = {});
        ~RequestTokenTo3PIDEmailJob() override;

        // Result properties

        /// An email was sent to the given address.
        const Sid& data() const;

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };

    /// Begins the validation process for a phone number for association with
    /// the user's account.
    ///
    /// Proxies the Identity Service API ``validate/msisdn/requestToken``, but
    /// first checks that the given phone number is **not** already associated
    /// with an account on this homeserver. This API should be used to request
    /// validation tokens when adding a phone number to an account. This API's
    /// parameters and response are identical to that of the
    /// |/register/msisdn/requestToken|_ endpoint.
    class RequestTokenTo3PIDMSISDNJob : public BaseJob
    {
        public:
        /*! Begins the validation process for a phone number for association with the user's account.
         * \param clientSecret
         *   A unique string generated by the client, and used to identify the
         *   validation attempt. It must be a string consisting of the
         * characters
         *   ``[0-9a-zA-Z.=_-]``. Its length must not exceed 255 characters and
         * it must not be empty. \param country The two-letter uppercase ISO
         * country code that the number in
         *   ``phone_number`` should be parsed as if it were dialled from.
         * \param phoneNumber
         *   The phone number to validate.
         * \param sendAttempt
         *   The server will only send an SMS if the ``send_attempt`` is a
         *   number greater than the most recent one which it has seen,
         *   scoped to that ``country`` + ``phone_number`` + ``client_secret``
         *   triple. This is to avoid repeatedly sending the same SMS in
         *   the case of request retries between the POSTing user and the
         *   identity server. The client should increment this value if
         *   they desire a new SMS (e.g. a reminder) to be sent.
         * \param idServer
         *   The hostname of the identity server to communicate with. May
         *   optionally include a port.
         * \param nextLink
         *   Optional. When the validation is completed, the identity
         *   server will redirect the user to this URL.
         */
        explicit RequestTokenTo3PIDMSISDNJob(const QString& clientSecret,
                                             const QString& country,
                                             const QString& phoneNumber,
                                             int sendAttempt,
                                             const QString& idServer,
                                             const QString& nextLink = {});
        ~RequestTokenTo3PIDMSISDNJob() override;

        // Result properties

        /// An SMS message was sent to the given phone number.
        const Sid& data() const;

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include <QtCore/QVector>

namespace QMatrixClient
{
    // Operations

    /// Gets a list of a user's third party identifiers.
    /// 
    /// Gets a list of the third party identifiers that the homeserver has
    /// associated with the user's account.
    /// 
    /// This is *not* the same as the list of third party identifiers bound to
    /// the user's Matrix ID in Identity Servers.
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
            /// This is *not* the same as the list of third party identifiers bound to
            /// the user's Matrix ID in Identity Servers.
            /// 
            /// Identifiers in this list may be used by the homeserver as, for example,
            /// identifiers that it will accept to reset the user's account password.
            struct ThirdPartyIdentifier
            {
        /// The medium of the third party identifier.
                QString medium;
        /// The third party identifier address.
                QString address;
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
            /// This is *not* the same as the list of third party identifiers bound to
            /// the user's Matrix ID in Identity Servers.
            /// 
            /// Identifiers in this list may be used by the homeserver as, for example,
            /// identifiers that it will accept to reset the user's account password.
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
            struct ThreePidCredentials
            {
        /// The client secret used in the session with the Identity Server.
                QString clientSecret;
        /// The Identity Server to use.
                QString idServer;
        /// The session identifier given by the Identity Server.
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
            explicit Post3PIDsJob(const ThreePidCredentials& threePidCreds, bool bind = false);
    };

    /// Requests a validation token be sent to the given email address for the purpose of adding an email address to an account
    /// 
    /// Proxies the identity server API ``validate/email/requestToken``, but
    /// first checks that the given email address is **not** already associated
    /// with an account on this Home Server. This API should be used to request
    /// validation tokens when adding an email address to an account. This API's
    /// parameters and response is identical to that of the HS API
    /// |/register/email/requestToken|_ endpoint.
    class RequestTokenTo3PIDJob : public BaseJob
    {
        public:
            explicit RequestTokenTo3PIDJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * RequestTokenTo3PIDJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

    };
} // namespace QMatrixClient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "converters.h"
#include <QtCore/QJsonObject>

namespace QMatrixClient
{
    // Operations

    /// Get an OpenID token object to verify the requester's identity.
    ///
    /// Gets an OpenID token object that the requester may supply to another
    /// service to verify their identity in Matrix. The generated token is only
    /// valid for exchanging for user information from the federation API for
    /// OpenID.
    /// 
    /// The access token generated is only valid for the OpenID API. It cannot
    /// be used to request another OpenID access token or call ``/sync``, for
    /// example.
    class RequestOpenIdTokenJob : public BaseJob
    {
        public:
            /*! Get an OpenID token object to verify the requester's identity.
             * \param userId
             *   The user to request and OpenID token for. Should be the user who
             *   is authenticated for the request.
             * \param body
             *   An empty object. Reserved for future expansion.
             */
            explicit RequestOpenIdTokenJob(const QString& userId, const QJsonObject& body = {});
            ~RequestOpenIdTokenJob() override;

            // Result properties

            /// An access token the consumer may use to verify the identity of
            /// the person who generated the token. This is given to the federation
            /// API ``GET /openid/userinfo``.
            const QString& accessToken() const;
            /// The string ``Bearer``.
            const QString& tokenType() const;
            /// The homeserver domain the consumer should use when attempting to
            /// verify the user's identity.
            const QString& matrixServerName() const;
            /// The number of seconds before this token expires and a new one must
            /// be generated.
            int expiresIn() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

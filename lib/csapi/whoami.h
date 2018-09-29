/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    /// Gets information about the owner of an access token.
    ///
    /// Gets information about the owner of a given access token. 
    /// 
    /// Note that, as with the rest of the Client-Server API, 
    /// Application Services may masquerade as users within their 
    /// namespace by giving a ``user_id`` query parameter. In this 
    /// situation, the server should verify that the given ``user_id``
    /// is registered by the appservice, and return it in the response 
    /// body.
    class GetTokenOwnerJob : public BaseJob
    {
        public:
            explicit GetTokenOwnerJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetTokenOwnerJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetTokenOwnerJob() override;

            // Result properties

            /// The user id that owns the access token.
            const QString& userId() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

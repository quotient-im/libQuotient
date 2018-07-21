/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "csapi/../application-service/definitions/user.h"
#include "csapi/../application-service/definitions/location.h"
#include <QtCore/QHash>
#include <QtCore/QVector>
#include "converters.h"
#include "csapi/../application-service/definitions/protocol.h"

namespace QMatrixClient
{
    // Operations

    /// Retrieve metadata about all protocols that a homeserver supports.
    /// 
    /// Fetches the overall metadata about protocols supported by the
    /// homeserver. Includes both the available protocols and all fields
    /// required for queries against each protocol.
    class GetProtocolsJob : public BaseJob
    {
        public:
            explicit GetProtocolsJob();

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetProtocolsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetProtocolsJob() override;

            // Result properties

            /// The protocols supported by the homeserver.
            const QHash<QString, ThirdPartyProtocol>& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Retrieve metadata about a specific protocol that the homeserver supports.
    /// 
    /// Fetches the metadata from the homeserver about a particular third party protocol.
    class GetProtocolMetadataJob : public BaseJob
    {
        public:
            /*! Retrieve metadata about a specific protocol that the homeserver supports.
             * \param protocol 
             *   The name of the protocol.
             */
            explicit GetProtocolMetadataJob(const QString& protocol);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetProtocolMetadataJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& protocol);

            ~GetProtocolMetadataJob() override;

            // Result properties

            /// The protocol was found and metadata returned.
            const ThirdPartyProtocol& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Retreive Matrix-side portals rooms leading to a third party location.
    /// 
    /// Requesting this endpoint with a valid protocol name results in a list
    /// of successful mapping results in a JSON array. Each result contains
    /// objects to represent the Matrix room or rooms that represent a portal
    /// to this third party network. Each has the Matrix room alias string,
    /// an identifier for the particular third party network protocol, and an
    /// object containing the network-specific fields that comprise this
    /// identifier. It should attempt to canonicalise the identifier as much
    /// as reasonably possible given the network type.
    class QueryLocationByProtocolJob : public BaseJob
    {
        public:
            /*! Retreive Matrix-side portals rooms leading to a third party location.
             * \param protocol 
             *   The protocol used to communicate to the third party network.
             * \param searchFields 
             *   One or more custom fields to help identify the third party
             *   location.
             */
            explicit QueryLocationByProtocolJob(const QString& protocol, const QString& searchFields = {});

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * QueryLocationByProtocolJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& protocol, const QString& searchFields = {});

            ~QueryLocationByProtocolJob() override;

            // Result properties

            /// At least one portal room was found.
            const QVector<ThirdPartyLocation>& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Retrieve the Matrix User ID of a corresponding third party user.
    /// 
    /// Retrieve a Matrix User ID linked to a user on the third party service, given
    /// a set of user parameters.
    class QueryUserByProtocolJob : public BaseJob
    {
        public:
            /*! Retrieve the Matrix User ID of a corresponding third party user.
             * \param protocol 
             *   The name of the protocol.
             * \param fields 
             *   One or more custom fields that are passed to the AS to help identify the user.
             */
            explicit QueryUserByProtocolJob(const QString& protocol, const QString& fields = {});

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * QueryUserByProtocolJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& protocol, const QString& fields = {});

            ~QueryUserByProtocolJob() override;

            // Result properties

            /// The Matrix User IDs found with the given parameters.
            const QVector<ThirdPartyUser>& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Reverse-lookup third party locations given a Matrix room alias.
    /// 
    /// Retreive an array of third party network locations from a Matrix room
    /// alias.
    class QueryLocationByAliasJob : public BaseJob
    {
        public:
            /*! Reverse-lookup third party locations given a Matrix room alias.
             * \param alias 
             *   The Matrix room alias to look up.
             */
            explicit QueryLocationByAliasJob(const QString& alias);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * QueryLocationByAliasJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& alias);

            ~QueryLocationByAliasJob() override;

            // Result properties

            /// All found third party locations.
            const QVector<ThirdPartyLocation>& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Reverse-lookup third party users given a Matrix User ID.
    /// 
    /// Retreive an array of third party users from a Matrix User ID.
    class QueryUserByIDJob : public BaseJob
    {
        public:
            /*! Reverse-lookup third party users given a Matrix User ID.
             * \param userid 
             *   The Matrix User ID to look up.
             */
            explicit QueryUserByIDJob(const QString& userid);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * QueryUserByIDJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userid);

            ~QueryUserByIDJob() override;

            // Result properties

            /// An array of third party users.
            const QVector<ThirdPartyUser>& data() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

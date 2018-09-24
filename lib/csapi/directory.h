/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"


namespace QMatrixClient
{
    // Operations

    /// Create a new mapping from room alias to room ID.
    class SetRoomAliasJob : public BaseJob
    {
        public:
            /*! Create a new mapping from room alias to room ID.
             * \param roomAlias
             *   The room alias to set.
             * \param roomId
             *   The room ID to set.
             */
            explicit SetRoomAliasJob(const QString& roomAlias, const QString& roomId = {});
    };

    /// Get the room ID corresponding to this room alias.
    ///
    /// Requests that the server resolve a room alias to a room ID.
    /// 
    /// The server will use the federation API to resolve the alias if the
    /// domain part of the alias does not correspond to the server's own
    /// domain.
    class GetRoomIdByAliasJob : public BaseJob
    {
        public:
            /*! Get the room ID corresponding to this room alias.
             * \param roomAlias
             *   The room alias.
             */
            explicit GetRoomIdByAliasJob(const QString& roomAlias);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetRoomIdByAliasJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomAlias);

            ~GetRoomIdByAliasJob() override;

            // Result properties

            /// The room ID for this room alias.
            const QString& roomId() const;
            /// A list of servers that are aware of this room alias.
            const QStringList& servers() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    /// Remove a mapping of room alias to room ID.
    ///
    /// Remove a mapping of room alias to room ID.
    /// 
    /// Servers may choose to implement additional access control checks here, for instance that room aliases can only be deleted by their creator or a server administrator.
    class DeleteRoomAliasJob : public BaseJob
    {
        public:
            /*! Remove a mapping of room alias to room ID.
             * \param roomAlias
             *   The room alias to remove.
             */
            explicit DeleteRoomAliasJob(const QString& roomAlias);

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * DeleteRoomAliasJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomAlias);

    };
} // namespace QMatrixClient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QVector>
#include "converters.h"

namespace QMatrixClient
{
    // Operations

    /// Searches the user directory.
    /// 
    /// This API performs a server-side search over all users registered on the server.
    /// It searches user ID and displayname case-insensitively for users that you share a room with or that are in public rooms.
    class SearchUserDirectoryJob : public BaseJob
    {
        public:
            // Inner data structures

            /// This API performs a server-side search over all users registered on the server.
            /// It searches user ID and displayname case-insensitively for users that you share a room with or that are in public rooms.
            struct User
            {
        /// The user's matrix user ID.
                QString userId;
        /// The display name of the user, if one exists.
                QString displayName;
        /// The avatar url, as an MXC, if one exists.
                QString avatarUrl;
            };

            // Construction/destruction

            /*! Searches the user directory.
             * \param searchTerm 
             *   The term to search for
             * \param limit 
             *   The maximum number of results to return (Defaults to 10).
             */
            explicit SearchUserDirectoryJob(const QString& searchTerm, Omittable<int> limit = none);
            ~SearchUserDirectoryJob() override;

            // Result properties

            /// Ordered by rank and then whether or not profile info is available.
            const QVector<User>& results() const;
            /// Indicates if the result list has been truncated by the limit.
            bool limited() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

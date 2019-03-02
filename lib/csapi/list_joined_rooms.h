/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

namespace QMatrixClient {
    // Operations

    /// Lists the user's current rooms.
    ///
    /// This API returns a list of the user's current rooms.
    class GetJoinedRoomsJob : public BaseJob
    {
        public:
        explicit GetJoinedRoomsJob();

        /*! Construct a URL without creating a full-fledged job object
         *
         * This function can be used when a URL for
         * GetJoinedRoomsJob is necessary but the job
         * itself isn't.
         */
        static QUrl makeRequestUrl(QUrl baseUrl);

        ~GetJoinedRoomsJob() override;

        // Result properties

        /// The ID of each room in which the user has ``joined`` membership.
        const QStringList& joinedRooms() const;

        protected:
        Status parseJson(const QJsonDocument& data) override;

        private:
        class Private;
        QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

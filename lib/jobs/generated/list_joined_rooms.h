/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QVector>


namespace QMatrixClient
{
    // Operations

    class GetJoinedRoomsJob : public BaseJob
    {
        public:
            explicit GetJoinedRoomsJob();

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetJoinedRoomsJob. This function can be used when
             * a URL for GetJoinedRoomsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl);

            ~GetJoinedRoomsJob() override;

            // Result properties

            const QVector<QString>& joinedRooms() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

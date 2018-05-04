/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include <QtCore/QJsonObject>


namespace QMatrixClient
{
    // Operations

    class GetRoomTagsJob : public BaseJob
    {
        public:
            explicit GetRoomTagsJob(const QString& userId, const QString& roomId);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetRoomTagsJob. This function can be used when
             * a URL for GetRoomTagsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& roomId);

            ~GetRoomTagsJob() override;

            // Result properties

            const QJsonObject& tags() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class SetRoomTagJob : public BaseJob
    {
        public:
            explicit SetRoomTagJob(const QString& userId, const QString& roomId, const QString& tag, const QJsonObject& body = {});
    };

    class DeleteRoomTagJob : public BaseJob
    {
        public:
            explicit DeleteRoomTagJob(const QString& userId, const QString& roomId, const QString& tag);

            /** Construct a URL out of baseUrl and usual parameters passed to
             * DeleteRoomTagJob. This function can be used when
             * a URL for DeleteRoomTagJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& userId, const QString& roomId, const QString& tag);

    };
} // namespace QMatrixClient

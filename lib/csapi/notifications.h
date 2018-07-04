/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "jobs/basejob.h"

#include "events/eventloader.h"
#include "converters.h"
#include <QtCore/QVector>
#include <QtCore/QVariant>
#include <QtCore/QJsonObject>

namespace QMatrixClient
{
    // Operations

    class GetNotificationsJob : public BaseJob
    {
        public:
            // Inner data structures

            struct Notification
            {
                QVector<QVariant> actions;
                EventPtr event;
                QString profileTag;
                bool read;
                QString roomId;
                qint64 ts;
            };

            // Construction/destruction

            explicit GetNotificationsJob(const QString& from = {}, Omittable<int> limit = none, const QString& only = {});

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetNotificationsJob. This function can be used when
             * a URL for GetNotificationsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& from = {}, Omittable<int> limit = none, const QString& only = {});

            ~GetNotificationsJob() override;

            // Result properties

            const QString& nextToken() const;
            std::vector<Notification>&& notifications();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

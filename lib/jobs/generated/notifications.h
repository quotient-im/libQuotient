/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <vector>
#include "events/event.h"
#include <QtCore/QJsonObject>
#include <QtCore/QVector>

#include "converters.h"

namespace QMatrixClient
{
    // Operations

    class GetNotificationsJob : public BaseJob
    {
        public:
            // Inner data structures

            using Event = EventPtr;

            struct Notification
            {
                QVector<QJsonObject> actions;
                Event event;
                QString profileTag;
                bool read;
                QString roomId;
                qint64 ts;
                
            };

            // End of inner data structures

            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetNotificationsJob. This function can be used when
             * a URL for GetNotificationsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& from = {}, int limit = {}, const QString& only = {});

            explicit GetNotificationsJob(const QString& from = {}, int limit = {}, const QString& only = {});
            ~GetNotificationsJob() override;

            const QString& nextToken() const;
            const std::vector<Notification>& notifications() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

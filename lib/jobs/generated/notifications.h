/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

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

            struct Unsigned
            {
                qint64 age;
                QJsonObject prevContent;
                QString transactionId;
                QJsonObject redactedBecause;
                
            };

            struct Event
            {
                QString eventId;
                QJsonObject content;
                qint64 originServerTimestamp;
                QString sender;
                QString stateKey;
                QString type;
                Unsigned unsignedData;
                
            };

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
            const QVector<Notification>& notifications() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

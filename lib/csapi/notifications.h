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

    /// Gets a list of events that the user has been notified about
    /// 
    /// This API is used to paginate through the list of events that the
    /// user has been, or would have been notified about.
    class GetNotificationsJob : public BaseJob
    {
        public:
            // Inner data structures

            /// This API is used to paginate through the list of events that the
            /// user has been, or would have been notified about.
            struct Notification
            {
                /// The action(s) to perform when the conditions for this rule are met.
                /// See `Push Rules: API`_.
                QVector<QVariant> actions;
                /// The Event object for the event that triggered the notification.
                EventPtr event;
                /// The profile tag of the rule that matched this event.
                QString profileTag;
                /// Indicates whether the user has sent a read receipt indicating
                /// that they have read this message.
                bool read;
                /// The ID of the room in which the event was posted.
                QString roomId;
                /// The unix timestamp at which the event notification was sent,
                /// in milliseconds.
                qint64 ts;
            };

            // Construction/destruction

            /*! Gets a list of events that the user has been notified about
             * \param from 
             *   Pagination token given to retrieve the next set of events.
             * \param limit 
             *   Limit on the number of events to return in this request.
             * \param only 
             *   Allows basic filtering of events returned. Supply ``highlight``
             *   to return only events where the notification had the highlight
             *   tweak set.
             */
            explicit GetNotificationsJob(const QString& from = {}, Omittable<int> limit = none, const QString& only = {});

            /*! Construct a URL without creating a full-fledged job object
             *
             * This function can be used when a URL for
             * GetNotificationsJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& from = {}, Omittable<int> limit = none, const QString& only = {});

            ~GetNotificationsJob() override;

            // Result properties

            /// The token to supply in the ``from`` param of the next
            /// ``/notifications`` request in order to request more
            /// events. If this is absent, there are no more results.
            const QString& nextToken() const;
            /// The list of events that triggered notifications.
            std::vector<Notification>&& notifications();

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };
} // namespace QMatrixClient

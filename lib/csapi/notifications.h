/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "events/eventloader.h"
#include "jobs/basejob.h"

namespace Quotient {

/*! \brief Gets a list of events that the user has been notified about
 *
 * This API is used to paginate through the list of events that the
 * user has been, or would have been notified about.
 */
class GetNotificationsJob : public BaseJob {
public:
    // Inner data structures

    /// This API is used to paginate through the list of events that the
    /// user has been, or would have been notified about.
    struct Notification {
        /// The action(s) to perform when the conditions for this rule are met.
        /// See [Push Rules: API](/client-server-api/#push-rules-api).
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

    /*! \brief Gets a list of events that the user has been notified about
     *
     * \param from
     *   Pagination token given to retrieve the next set of events.
     *
     * \param limit
     *   Limit on the number of events to return in this request.
     *
     * \param only
     *   Allows basic filtering of events returned. Supply `highlight`
     *   to return only events where the notification had the highlight
     *   tweak set.
     */
    explicit GetNotificationsJob(const QString& from = {},
                                 Omittable<int> limit = none,
                                 const QString& only = {});

    /*! \brief Construct a URL without creating a full-fledged job object
     *
     * This function can be used when a URL for GetNotificationsJob
     * is necessary but the job itself isn't.
     */
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& from = {},
                               Omittable<int> limit = none,
                               const QString& only = {});

    // Result properties

    /// The token to supply in the `from` param of the next
    /// `/notifications` request in order to request more
    /// events. If this is absent, there are no more results.
    QString nextToken() const { return loadFromJson<QString>("next_token"_ls); }

    /// The list of events that triggered notifications.
    std::vector<Notification> notifications()
    {
        return takeFromJson<std::vector<Notification>>("notifications"_ls);
    }
};

template <>
struct JsonObjectConverter<GetNotificationsJob::Notification> {
    static void fillFrom(const QJsonObject& jo,
                         GetNotificationsJob::Notification& result)
    {
        fromJson(jo.value("actions"_ls), result.actions);
        fromJson(jo.value("event"_ls), result.event);
        fromJson(jo.value("profile_tag"_ls), result.profileTag);
        fromJson(jo.value("read"_ls), result.read);
        fromJson(jo.value("room_id"_ls), result.roomId);
        fromJson(jo.value("ts"_ls), result.ts);
    }
};

} // namespace Quotient

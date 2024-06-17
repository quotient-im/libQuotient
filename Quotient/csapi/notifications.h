// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/events/event.h>
#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Gets a list of events that the user has been notified about
//!
//! This API is used to paginate through the list of events that the
//! user has been, or would have been notified about.
class QUOTIENT_API GetNotificationsJob : public BaseJob {
public:
    // Inner data structures

    struct QUOTIENT_API Notification {
        //! The action(s) to perform when the conditions for this rule are met.
        //! See [Push Rules: API](/client-server-api/#push-rules-api).
        QVector<QVariant> actions;

        //! The Event object for the event that triggered the notification.
        EventPtr event;

        //! Indicates whether the user has sent a read receipt indicating
        //! that they have read this message.
        bool read;

        //! The ID of the room in which the event was posted.
        QString roomId;

        //! The unix timestamp at which the event notification was sent,
        //! in milliseconds.
        qint64 ts;

        //! The profile tag of the rule that matched this event.
        QString profileTag{};
    };

    // Construction/destruction

    //! \param from
    //!   Pagination token to continue from. This should be the `next_token`
    //!   returned from an earlier call to this endpoint.
    //!
    //! \param limit
    //!   Limit on the number of events to return in this request.
    //!
    //! \param only
    //!   Allows basic filtering of events returned. Supply `highlight`
    //!   to return only events where the notification had the highlight
    //!   tweak set.
    explicit GetNotificationsJob(const QString& from = {}, std::optional<int> limit = std::nullopt,
                                 const QString& only = {});

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetNotificationsJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl, const QString& from = {},
                               std::optional<int> limit = std::nullopt, const QString& only = {});

    // Result properties

    //! The token to supply in the `from` param of the next
    //! `/notifications` request in order to request more
    //! events. If this is absent, there are no more results.
    QString nextToken() const { return loadFromJson<QString>("next_token"_ls); }

    //! The list of events that triggered notifications.
    std::vector<Notification> notifications()
    {
        return takeFromJson<std::vector<Notification>>("notifications"_ls);
    }

    struct Response {
        //! The token to supply in the `from` param of the next
        //! `/notifications` request in order to request more
        //! events. If this is absent, there are no more results.
        QString nextToken{};

        //! The list of events that triggered notifications.
        std::vector<Notification> notifications{};
    };
};

template <std::derived_from<GetNotificationsJob> JobT>
constexpr inline auto doCollectResponse<JobT> =
    [](JobT* j) -> GetNotificationsJob::Response { return { j->nextToken(), j->notifications() }; };

template <>
struct QUOTIENT_API JsonObjectConverter<GetNotificationsJob::Notification> {
    static void fillFrom(const QJsonObject& jo, GetNotificationsJob::Notification& result)
    {
        fillFromJson(jo.value("actions"_ls), result.actions);
        fillFromJson(jo.value("event"_ls), result.event);
        fillFromJson(jo.value("read"_ls), result.read);
        fillFromJson(jo.value("room_id"_ls), result.roomId);
        fillFromJson(jo.value("ts"_ls), result.ts);
        fillFromJson(jo.value("profile_tag"_ls), result.profileTag);
    }
};

} // namespace Quotient

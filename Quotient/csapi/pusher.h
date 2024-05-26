// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#pragma once

#include <Quotient/jobs/basejob.h>

namespace Quotient {

//! \brief Gets the current pushers for the authenticated user
//!
//! Gets all currently active pushers for the authenticated user.
class QUOTIENT_API GetPushersJob : public BaseJob {
public:
    // Inner data structures

    //! A dictionary of information for the pusher implementation
    //! itself.
    struct QUOTIENT_API PusherData {
        //! Required if `kind` is `http`. The URL to use to send
        //! notifications to.
        QUrl url{};

        //! The format to use when sending notifications to the Push
        //! Gateway.
        QString format{};
    };

    struct QUOTIENT_API Pusher {
        //! This is a unique identifier for this pusher. See `/set` for
        //! more detail.
        //! Max length, 512 bytes.
        QString pushkey;

        //! The kind of pusher. `"http"` is a pusher that
        //! sends HTTP pokes.
        QString kind;

        //! This is a reverse-DNS style identifier for the application.
        //! Max length, 64 chars.
        QString appId;

        //! A string that will allow the user to identify what application
        //! owns this pusher.
        QString appDisplayName;

        //! A string that will allow the user to identify what device owns
        //! this pusher.
        QString deviceDisplayName;

        //! The preferred language for receiving notifications (e.g. 'en'
        //! or 'en-US')
        QString lang;

        //! A dictionary of information for the pusher implementation
        //! itself.
        PusherData data;

        //! This string determines which set of device specific rules this
        //! pusher executes.
        QString profileTag{};
    };

    // Construction/destruction

    explicit GetPushersJob();

    //! \brief Construct a URL without creating a full-fledged job object
    //!
    //! This function can be used when a URL for GetPushersJob
    //! is necessary but the job itself isn't.
    static QUrl makeRequestUrl(QUrl baseUrl);

    // Result properties

    //! An array containing the current pushers for the user
    QVector<Pusher> pushers() const { return loadFromJson<QVector<Pusher>>("pushers"_ls); }
};

inline auto collectResponse(const GetPushersJob* job) { return job->pushers(); }

template <>
struct QUOTIENT_API JsonObjectConverter<GetPushersJob::PusherData> {
    static void fillFrom(const QJsonObject& jo, GetPushersJob::PusherData& result)
    {
        fillFromJson(jo.value("url"_ls), result.url);
        fillFromJson(jo.value("format"_ls), result.format);
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<GetPushersJob::Pusher> {
    static void fillFrom(const QJsonObject& jo, GetPushersJob::Pusher& result)
    {
        fillFromJson(jo.value("pushkey"_ls), result.pushkey);
        fillFromJson(jo.value("kind"_ls), result.kind);
        fillFromJson(jo.value("app_id"_ls), result.appId);
        fillFromJson(jo.value("app_display_name"_ls), result.appDisplayName);
        fillFromJson(jo.value("device_display_name"_ls), result.deviceDisplayName);
        fillFromJson(jo.value("lang"_ls), result.lang);
        fillFromJson(jo.value("data"_ls), result.data);
        fillFromJson(jo.value("profile_tag"_ls), result.profileTag);
    }
};

//! \brief Modify a pusher for this user on the homeserver.
//!
//! This endpoint allows the creation, modification and deletion of
//! [pushers](/client-server-api/#push-notifications) for this user ID. The behaviour of this
//! endpoint varies depending on the values in the JSON body.
//!
//! If `kind` is not `null`, the pusher with this `app_id` and `pushkey`
//! for this user is updated, or it is created if it doesn't exist. If
//! `kind` is `null`, the pusher with this `app_id` and `pushkey` for this
//! user is deleted.
class QUOTIENT_API PostPusherJob : public BaseJob {
public:
    // Inner data structures

    //! Required if `kind` is not `null`. A dictionary of information
    //! for the pusher implementation itself. If `kind` is `http`,
    //! this should contain `url` which is the URL to use to send
    //! notifications to.
    struct QUOTIENT_API PusherData {
        //! Required if `kind` is `http`. The URL to use to send
        //! notifications to. MUST be an HTTPS URL with a path of
        //! `/_matrix/push/v1/notify`.
        QUrl url{};

        //! The format to send notifications in to Push Gateways if the
        //! `kind` is `http`. The details about what fields the
        //! homeserver should send to the push gateway are defined in the
        //! [Push Gateway Specification](/push-gateway-api/). Currently the only format
        //! available is 'event_id_only'.
        QString format{};
    };

    // Construction/destruction

    //! \param pushkey
    //!   This is a unique identifier for this pusher. The value you
    //!   should use for this is the routing or destination address
    //!   information for the notification, for example, the APNS token
    //!   for APNS or the Registration ID for GCM. If your notification
    //!   client has no such concept, use any unique identifier.
    //!   Max length, 512 bytes.
    //!
    //!   If the `kind` is `"email"`, this is the email address to
    //!   send notifications to.
    //!
    //! \param kind
    //!   The kind of pusher to configure. `"http"` makes a pusher that
    //!   sends HTTP pokes. `"email"` makes a pusher that emails the
    //!   user with unread notifications. `null` deletes the pusher.
    //!
    //! \param appId
    //!   This is a reverse-DNS style identifier for the application.
    //!   It is recommended that this end with the platform, such that
    //!   different platform versions get different app identifiers.
    //!   Max length, 64 chars.
    //!
    //!   If the `kind` is `"email"`, this is `"m.email"`.
    //!
    //! \param appDisplayName
    //!   Required if `kind` is not `null`. A string that will allow the
    //!   user to identify what application owns this pusher.
    //!
    //! \param deviceDisplayName
    //!   Required if `kind` is not `null`. A string that will allow the
    //!   user to identify what device owns this pusher.
    //!
    //! \param profileTag
    //!   This string determines which set of device specific rules this
    //!   pusher executes.
    //!
    //! \param lang
    //!   Required if `kind` is not `null`. The preferred language for
    //!   receiving notifications (e.g. 'en' or 'en-US').
    //!
    //! \param data
    //!   Required if `kind` is not `null`. A dictionary of information
    //!   for the pusher implementation itself. If `kind` is `http`,
    //!   this should contain `url` which is the URL to use to send
    //!   notifications to.
    //!
    //! \param append
    //!   If true, the homeserver should add another pusher with the
    //!   given pushkey and App ID in addition to any others with
    //!   different user IDs. Otherwise, the homeserver must remove any
    //!   other pushers with the same App ID and pushkey for different
    //!   users. The default is `false`.
    explicit PostPusherJob(const QString& pushkey, const QString& kind, const QString& appId,
                           const QString& appDisplayName = {}, const QString& deviceDisplayName = {},
                           const QString& profileTag = {}, const QString& lang = {},
                           const std::optional<PusherData>& data = std::nullopt,
                           std::optional<bool> append = std::nullopt);
};

template <>
struct QUOTIENT_API JsonObjectConverter<PostPusherJob::PusherData> {
    static void dumpTo(QJsonObject& jo, const PostPusherJob::PusherData& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("url"), pod.url);
        addParam<IfNotEmpty>(jo, QStringLiteral("format"), pod.format);
    }
};

} // namespace Quotient

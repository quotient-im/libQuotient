/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "notifications.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    QJsonObject toJson(const GetNotificationsJob::Unsigned& pod)
    {
        QJsonObject o;
        o.insert("age", toJson(pod.age));
        o.insert("prev_content", toJson(pod.prevContent));
        o.insert("transaction_id", toJson(pod.transactionId));
        o.insert("redacted_because", toJson(pod.redactedBecause));
        
        return o;
    }

    template <> struct FromJson<GetNotificationsJob::Unsigned>
    {
        GetNotificationsJob::Unsigned operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            GetNotificationsJob::Unsigned result;
            result.age =
                fromJson<qint64>(o.value("age"));
            result.prevContent =
                fromJson<QJsonObject>(o.value("prev_content"));
            result.transactionId =
                fromJson<QString>(o.value("transaction_id"));
            result.redactedBecause =
                fromJson<QJsonObject>(o.value("redacted_because"));
            
            return result;
        }
    };
} // namespace QMatrixClient

namespace QMatrixClient
{
    QJsonObject toJson(const GetNotificationsJob::Event& pod)
    {
        QJsonObject o;
        o.insert("event_id", toJson(pod.eventId));
        o.insert("content", toJson(pod.content));
        o.insert("origin_server_ts", toJson(pod.originServerTimestamp));
        o.insert("sender", toJson(pod.sender));
        o.insert("state_key", toJson(pod.stateKey));
        o.insert("type", toJson(pod.type));
        o.insert("unsigned", toJson(pod.unsignedData));
        
        return o;
    }

    template <> struct FromJson<GetNotificationsJob::Event>
    {
        GetNotificationsJob::Event operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            GetNotificationsJob::Event result;
            result.eventId =
                fromJson<QString>(o.value("event_id"));
            result.content =
                fromJson<QJsonObject>(o.value("content"));
            result.originServerTimestamp =
                fromJson<qint64>(o.value("origin_server_ts"));
            result.sender =
                fromJson<QString>(o.value("sender"));
            result.stateKey =
                fromJson<QString>(o.value("state_key"));
            result.type =
                fromJson<QString>(o.value("type"));
            result.unsignedData =
                fromJson<GetNotificationsJob::Unsigned>(o.value("unsigned"));
            
            return result;
        }
    };
} // namespace QMatrixClient

namespace QMatrixClient
{
    QJsonObject toJson(const GetNotificationsJob::Notification& pod)
    {
        QJsonObject o;
        o.insert("actions", toJson(pod.actions));
        o.insert("event", toJson(pod.event));
        o.insert("profile_tag", toJson(pod.profileTag));
        o.insert("read", toJson(pod.read));
        o.insert("room_id", toJson(pod.roomId));
        o.insert("ts", toJson(pod.ts));
        
        return o;
    }

    template <> struct FromJson<GetNotificationsJob::Notification>
    {
        GetNotificationsJob::Notification operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            GetNotificationsJob::Notification result;
            result.actions =
                fromJson<QVector<QJsonObject>>(o.value("actions"));
            result.event =
                fromJson<GetNotificationsJob::Event>(o.value("event"));
            result.profileTag =
                fromJson<QString>(o.value("profile_tag"));
            result.read =
                fromJson<bool>(o.value("read"));
            result.roomId =
                fromJson<QString>(o.value("room_id"));
            result.ts =
                fromJson<qint64>(o.value("ts"));
            
            return result;
        }
    };
} // namespace QMatrixClient

class GetNotificationsJob::Private
{
    public:
        QString nextToken;
        QVector<Notification> notifications;
};

BaseJob::Query queryToGetNotifications(const QString& from, int limit, const QString& only)
{
    BaseJob::Query _q;
    if (!from.isEmpty())
        _q.addQueryItem("from", from);
    _q.addQueryItem("limit", QString("%1").arg(limit));
    if (!only.isEmpty())
        _q.addQueryItem("only", only);
    return _q;
}

QUrl GetNotificationsJob::makeRequestUrl(QUrl baseUrl, const QString& from, int limit, const QString& only)
{
    return BaseJob::makeRequestUrl(baseUrl,
            basePath % "/notifications",
            queryToGetNotifications(from, limit, only));
}

GetNotificationsJob::GetNotificationsJob(const QString& from, int limit, const QString& only)
    : BaseJob(HttpVerb::Get, "GetNotificationsJob",
        basePath % "/notifications",
        queryToGetNotifications(from, limit, only))
    , d(new Private)
{
}

GetNotificationsJob::~GetNotificationsJob() = default;

const QString& GetNotificationsJob::nextToken() const
{
    return d->nextToken;
}

const QVector<GetNotificationsJob::Notification>& GetNotificationsJob::notifications() const
{
    return d->notifications;
}

BaseJob::Status GetNotificationsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->nextToken = fromJson<QString>(json.value("next_token"));
    if (!json.contains("notifications"))
        return { JsonParseError,
            "The key 'notifications' not found in the response" };
    d->notifications = fromJson<QVector<Notification>>(json.value("notifications"));
    return Success;
}


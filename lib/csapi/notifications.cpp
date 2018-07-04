/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "notifications.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    template <> struct FromJson<GetNotificationsJob::Notification>
    {
        GetNotificationsJob::Notification operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            GetNotificationsJob::Notification result;
            result.actions =
                fromJson<QVector<QVariant>>(_json.value("actions"_ls));
            result.event =
                fromJson<EventPtr>(_json.value("event"_ls));
            result.profileTag =
                fromJson<QString>(_json.value("profile_tag"_ls));
            result.read =
                fromJson<bool>(_json.value("read"_ls));
            result.roomId =
                fromJson<QString>(_json.value("room_id"_ls));
            result.ts =
                fromJson<qint64>(_json.value("ts"_ls));

            return result;
        }
    };
} // namespace QMatrixClient

class GetNotificationsJob::Private
{
    public:
        QString nextToken;
        std::vector<Notification> notifications;
};

BaseJob::Query queryToGetNotifications(const QString& from, Omittable<int> limit, const QString& only)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("only"), only);
    return _q;
}

QUrl GetNotificationsJob::makeRequestUrl(QUrl baseUrl, const QString& from, Omittable<int> limit, const QString& only)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/notifications",
            queryToGetNotifications(from, limit, only));
}

static const auto GetNotificationsJobName = QStringLiteral("GetNotificationsJob");

GetNotificationsJob::GetNotificationsJob(const QString& from, Omittable<int> limit, const QString& only)
    : BaseJob(HttpVerb::Get, GetNotificationsJobName,
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

std::vector<GetNotificationsJob::Notification>&& GetNotificationsJob::notifications()
{
    return std::move(d->notifications);
}

BaseJob::Status GetNotificationsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->nextToken = fromJson<QString>(json.value("next_token"_ls));
    if (!json.contains("notifications"_ls))
        return { JsonParseError,
            "The key 'notifications' not found in the response" };
    d->notifications = fromJson<std::vector<Notification>>(json.value("notifications"_ls));
    return Success;
}


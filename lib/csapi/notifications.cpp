/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "notifications.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient {
    // Converters

    template <> struct JsonObjectConverter<GetNotificationsJob::Notification> {
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
} // namespace QMatrixClient

class GetNotificationsJob::Private
{
    public:
    QString nextToken;
    std::vector<Notification> notifications;
};

BaseJob::Query queryToGetNotifications(const QString& from,
                                       Omittable<int> limit,
                                       const QString& only)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("only"), only);
    return _q;
}

QUrl GetNotificationsJob::makeRequestUrl(QUrl baseUrl, const QString& from,
                                         Omittable<int> limit,
                                         const QString& only)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/notifications",
                                   queryToGetNotifications(from, limit, only));
}

static const auto GetNotificationsJobName =
        QStringLiteral("GetNotificationsJob");

GetNotificationsJob::GetNotificationsJob(const QString& from,
                                         Omittable<int> limit,
                                         const QString& only)
    : BaseJob(HttpVerb::Get, GetNotificationsJobName,
              basePath % "/notifications",
              queryToGetNotifications(from, limit, only)),
      d(new Private)
{
}

GetNotificationsJob::~GetNotificationsJob() = default;

const QString& GetNotificationsJob::nextToken() const { return d->nextToken; }

std::vector<GetNotificationsJob::Notification>&&
GetNotificationsJob::notifications()
{
    return std::move(d->notifications);
}

BaseJob::Status GetNotificationsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("next_token"_ls), d->nextToken);
    if (!json.contains("notifications"_ls))
        return { JsonParseError,
                 "The key 'notifications' not found in the response" };
    fromJson(json.value("notifications"_ls), d->notifications);
    return Success;
}

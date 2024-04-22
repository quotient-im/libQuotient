// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "notifications.h"

using namespace Quotient;

auto queryToGetNotifications(const QString& from, std::optional<int> limit, const QString& only)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("only"), only);
    return _q;
}

QUrl GetNotificationsJob::makeRequestUrl(QUrl baseUrl, const QString& from,
                                         std::optional<int> limit, const QString& only)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/notifications"),
                                   queryToGetNotifications(from, limit, only));
}

GetNotificationsJob::GetNotificationsJob(const QString& from, std::optional<int> limit,
                                         const QString& only)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetNotificationsJob"),
              makePath("/_matrix/client/v3", "/notifications"),
              queryToGetNotifications(from, limit, only))
{
    addExpectedKey("notifications");
}

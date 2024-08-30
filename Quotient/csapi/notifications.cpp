// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "notifications.h"

using namespace Quotient;

auto queryToGetNotifications(const QString& from, std::optional<int> limit, const QString& only)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"from"_s, from);
    addParam<IfNotEmpty>(_q, u"limit"_s, limit);
    addParam<IfNotEmpty>(_q, u"only"_s, only);
    return _q;
}

QUrl GetNotificationsJob::makeRequestUrl(const HomeserverData& hsData, const QString& from,
                                         std::optional<int> limit, const QString& only)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/notifications"),
                                   queryToGetNotifications(from, limit, only));
}

GetNotificationsJob::GetNotificationsJob(const QString& from, std::optional<int> limit,
                                         const QString& only)
    : BaseJob(HttpVerb::Get, u"GetNotificationsJob"_s,
              makePath("/_matrix/client/v3", "/notifications"),
              queryToGetNotifications(from, limit, only))
{
    addExpectedKey("notifications");
}

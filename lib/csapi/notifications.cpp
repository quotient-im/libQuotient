/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "notifications.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

auto queryToGetNotifications(const QString& from, Omittable<int> limit,
                             const QString& only)
{
    QUrlQuery _q;
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
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/notifications",
                                   queryToGetNotifications(from, limit, only));
}

GetNotificationsJob::GetNotificationsJob(const QString& from,
                                         Omittable<int> limit,
                                         const QString& only)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetNotificationsJob"),
              QStringLiteral("/_matrix/client/r0") % "/notifications",
              queryToGetNotifications(from, limit, only))
{
    addExpectedKey("notifications");
}

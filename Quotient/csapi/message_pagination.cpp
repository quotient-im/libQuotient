// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "message_pagination.h"

using namespace Quotient;

auto queryToGetRoomEvents(const QString& from, const QString& to, const QString& dir,
                          Omittable<int> limit, const QString& filter)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("to"), to);
    addParam<>(_q, QStringLiteral("dir"), dir);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("filter"), filter);
    return _q;
}

QUrl GetRoomEventsJob::makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& dir,
                                      const QString& from, const QString& to, Omittable<int> limit,
                                      const QString& filter)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/rooms/", roomId, "/messages"),
                                   queryToGetRoomEvents(from, to, dir, limit, filter));
}

GetRoomEventsJob::GetRoomEventsJob(const QString& roomId, const QString& dir, const QString& from,
                                   const QString& to, Omittable<int> limit, const QString& filter)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRoomEventsJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/messages"),
              queryToGetRoomEvents(from, to, dir, limit, filter))
{
    addExpectedKey("start");
    addExpectedKey("chunk");
}

// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "message_pagination.h"

using namespace Quotient;

auto queryToGetRoomEvents(const QString& from, const QString& to, const QString& dir,
                          std::optional<int> limit, const QString& filter)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"from"_s, from);
    addParam<IfNotEmpty>(_q, u"to"_s, to);
    addParam<>(_q, u"dir"_s, dir);
    addParam<IfNotEmpty>(_q, u"limit"_s, limit);
    addParam<IfNotEmpty>(_q, u"filter"_s, filter);
    return _q;
}

QUrl GetRoomEventsJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                      const QString& dir, const QString& from, const QString& to,
                                      std::optional<int> limit, const QString& filter)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/rooms/", roomId, "/messages"),
                                   queryToGetRoomEvents(from, to, dir, limit, filter));
}

GetRoomEventsJob::GetRoomEventsJob(const QString& roomId, const QString& dir, const QString& from,
                                   const QString& to, std::optional<int> limit,
                                   const QString& filter)
    : BaseJob(HttpVerb::Get, u"GetRoomEventsJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/messages"),
              queryToGetRoomEvents(from, to, dir, limit, filter))
{
    addExpectedKey("start");
    addExpectedKey("chunk");
}

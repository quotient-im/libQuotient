// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "room_event_by_timestamp.h"

using namespace Quotient;

auto queryToGetEventByTimestamp(int ts, const QString& dir)
{
    QUrlQuery _q;
    addParam<>(_q, u"ts"_s, ts);
    addParam<>(_q, u"dir"_s, dir);
    return _q;
}

QUrl GetEventByTimestampJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                            int ts, const QString& dir)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v1", "/rooms/", roomId,
                                            "/timestamp_to_event"),
                                   queryToGetEventByTimestamp(ts, dir));
}

GetEventByTimestampJob::GetEventByTimestampJob(const QString& roomId, int ts, const QString& dir)
    : BaseJob(HttpVerb::Get, u"GetEventByTimestampJob"_s,
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/timestamp_to_event"),
              queryToGetEventByTimestamp(ts, dir))
{
    addExpectedKey(u"event_id"_s);
    addExpectedKey(u"origin_server_ts"_s);
}

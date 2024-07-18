// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "room_event_by_timestamp.h"

using namespace Quotient;

auto queryToGetEventByTimestamp(int ts, const QString& dir)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("ts"), ts);
    addParam<>(_q, QStringLiteral("dir"), dir);
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
    : BaseJob(HttpVerb::Get, QStringLiteral("GetEventByTimestampJob"),
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/timestamp_to_event"),
              queryToGetEventByTimestamp(ts, dir))
{
    addExpectedKey("event_id");
    addExpectedKey("origin_server_ts");
}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "message_pagination.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetRoomEventsJob::Private
{
    public:
        QString begin;
        QString end;
        RoomEvents chunk;
};

BaseJob::Query queryToGetRoomEvents(const QString& from, const QString& to, const QString& dir, Omittable<int> limit, const QString& filter)
{
    BaseJob::Query _q;
    _q.addQueryItem("from", from);
    if (!to.isEmpty())
        _q.addQueryItem("to", to);
    _q.addQueryItem("dir", dir);
    _q.addQueryItem("limit", QString("%1").arg(limit));
    if (!filter.isEmpty())
        _q.addQueryItem("filter", filter);
    return _q;
}

QUrl GetRoomEventsJob::makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& from, const QString& dir, const QString& to, Omittable<int> limit, const QString& filter)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/rooms/" % roomId % "/messages",
            queryToGetRoomEvents(from, to, dir, limit, filter));
}

GetRoomEventsJob::GetRoomEventsJob(const QString& roomId, const QString& from, const QString& dir, const QString& to, Omittable<int> limit, const QString& filter)
    : BaseJob(HttpVerb::Get, "GetRoomEventsJob",
        basePath % "/rooms/" % roomId % "/messages",
        queryToGetRoomEvents(from, to, dir, limit, filter))
    , d(new Private)
{
}

GetRoomEventsJob::~GetRoomEventsJob() = default;

const QString& GetRoomEventsJob::begin() const
{
    return d->begin;
}

const QString& GetRoomEventsJob::end() const
{
    return d->end;
}

RoomEvents&& GetRoomEventsJob::chunk()
{
    return std::move(d->chunk);
}

BaseJob::Status GetRoomEventsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->begin = fromJson<QString>(json.value("start"));
    d->end = fromJson<QString>(json.value("end"));
    d->chunk = fromJson<RoomEvents>(json.value("chunk"));
    return Success;
}


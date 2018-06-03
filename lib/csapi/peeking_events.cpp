/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "peeking_events.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class PeekEventsJob::Private
{
    public:
        QString begin;
        QString end;
        RoomEvents chunk;
};

BaseJob::Query queryToPeekEvents(const QString& from, Omittable<int> timeout, const QString& roomId)
{
    BaseJob::Query _q;
    if (!from.isEmpty())
        _q.addQueryItem("from", from);
    if (timeout)
        _q.addQueryItem("timeout", QString("%1").arg(timeout.value()));
    if (!roomId.isEmpty())
        _q.addQueryItem("room_id", roomId);
    return _q;
}

QUrl PeekEventsJob::makeRequestUrl(QUrl baseUrl, const QString& from, Omittable<int> timeout, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/events",
            queryToPeekEvents(from, timeout, roomId));
}

PeekEventsJob::PeekEventsJob(const QString& from, Omittable<int> timeout, const QString& roomId)
    : BaseJob(HttpVerb::Get, "PeekEventsJob",
        basePath % "/events",
        queryToPeekEvents(from, timeout, roomId))
    , d(new Private)
{
}

PeekEventsJob::~PeekEventsJob() = default;

const QString& PeekEventsJob::begin() const
{
    return d->begin;
}

const QString& PeekEventsJob::end() const
{
    return d->end;
}

RoomEvents&& PeekEventsJob::chunk()
{
    return std::move(d->chunk);
}

BaseJob::Status PeekEventsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->begin = fromJson<QString>(json.value("start"));
    d->end = fromJson<QString>(json.value("end"));
    d->chunk = fromJson<RoomEvents>(json.value("chunk"));
    return Success;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "peeking_events.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class PeekEventsJob::Private {
public:
    QString begin;
    QString end;
    RoomEvents chunk;
};

BaseJob::Query queryToPeekEvents(const QString& from, Omittable<int> timeout,
                                 const QString& roomId)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("timeout"), timeout);
    addParam<IfNotEmpty>(_q, QStringLiteral("room_id"), roomId);
    return _q;
}

QUrl PeekEventsJob::makeRequestUrl(QUrl baseUrl, const QString& from,
                                   Omittable<int> timeout, const QString& roomId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/events",
                                   queryToPeekEvents(from, timeout, roomId));
}

PeekEventsJob::PeekEventsJob(const QString& from, Omittable<int> timeout,
                             const QString& roomId)
    : BaseJob(HttpVerb::Get, QStringLiteral("PeekEventsJob"),
              basePath % "/events", queryToPeekEvents(from, timeout, roomId))
    , d(new Private)
{}

PeekEventsJob::~PeekEventsJob() = default;

const QString& PeekEventsJob::begin() const { return d->begin; }

const QString& PeekEventsJob::end() const { return d->end; }

RoomEvents&& PeekEventsJob::chunk() { return std::move(d->chunk); }

BaseJob::Status PeekEventsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("start"_ls), d->begin);
    fromJson(json.value("end"_ls), d->end);
    fromJson(json.value("chunk"_ls), d->chunk);

    return Success;
}

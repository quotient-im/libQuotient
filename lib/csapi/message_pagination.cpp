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

BaseJob::Query queryToGetRoomEvents(const QString& from, const QString& to,
                                    const QString& dir, Omittable<int> limit,
                                    const QString& filter)
{
    BaseJob::Query _q;
    addParam<>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("to"), to);
    addParam<>(_q, QStringLiteral("dir"), dir);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("filter"), filter);
    return _q;
}

QUrl GetRoomEventsJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                      const QString& from, const QString& dir,
                                      const QString& to, Omittable<int> limit,
                                      const QString& filter)
{
    return BaseJob::makeRequestUrl(
            std::move(baseUrl), basePath % "/rooms/" % roomId % "/messages",
            queryToGetRoomEvents(from, to, dir, limit, filter));
}

static const auto GetRoomEventsJobName = QStringLiteral("GetRoomEventsJob");

GetRoomEventsJob::GetRoomEventsJob(const QString& roomId, const QString& from,
                                   const QString& dir, const QString& to,
                                   Omittable<int> limit, const QString& filter)
    : BaseJob(HttpVerb::Get, GetRoomEventsJobName,
              basePath % "/rooms/" % roomId % "/messages",
              queryToGetRoomEvents(from, to, dir, limit, filter)),
      d(new Private)
{
}

GetRoomEventsJob::~GetRoomEventsJob() = default;

const QString& GetRoomEventsJob::begin() const { return d->begin; }

const QString& GetRoomEventsJob::end() const { return d->end; }

RoomEvents&& GetRoomEventsJob::chunk() { return std::move(d->chunk); }

BaseJob::Status GetRoomEventsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("start"_ls), d->begin);
    fromJson(json.value("end"_ls), d->end);
    fromJson(json.value("chunk"_ls), d->chunk);
    return Success;
}

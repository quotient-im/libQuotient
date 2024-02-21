// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "event_context.h"

using namespace Quotient;

auto queryToGetEventContext(Omittable<int> limit, const QString& filter)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("filter"), filter);
    return _q;
}

QUrl GetEventContextJob::makeRequestUrl(QUrl baseUrl, const QString& roomId, const QString& eventId,
                                        Omittable<int> limit, const QString& filter)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/rooms/", roomId, "/context/",
                                            eventId),
                                   queryToGetEventContext(limit, filter));
}

GetEventContextJob::GetEventContextJob(const QString& roomId, const QString& eventId,
                                       Omittable<int> limit, const QString& filter)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetEventContextJob"),
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/context/", eventId),
              queryToGetEventContext(limit, filter))
{}

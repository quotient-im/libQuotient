// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "event_context.h"

using namespace Quotient;

auto queryToGetEventContext(std::optional<int> limit, const QString& filter)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"limit"_s, limit);
    addParam<IfNotEmpty>(_q, u"filter"_s, filter);
    return _q;
}

QUrl GetEventContextJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                        const QString& eventId, std::optional<int> limit,
                                        const QString& filter)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v3", "/rooms/", roomId, "/context/",
                                            eventId),
                                   queryToGetEventContext(limit, filter));
}

GetEventContextJob::GetEventContextJob(const QString& roomId, const QString& eventId,
                                       std::optional<int> limit, const QString& filter)
    : BaseJob(HttpVerb::Get, u"GetEventContextJob"_s,
              makePath("/_matrix/client/v3", "/rooms/", roomId, "/context/", eventId),
              queryToGetEventContext(limit, filter))
{}

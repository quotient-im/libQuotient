// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "relations.h"

using namespace Quotient;

auto queryToGetRelatingEvents(const QString& from, const QString& to, std::optional<int> limit,
                              const QString& dir, std::optional<bool> recurse)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"from"_s, from);
    addParam<IfNotEmpty>(_q, u"to"_s, to);
    addParam<IfNotEmpty>(_q, u"limit"_s, limit);
    addParam<IfNotEmpty>(_q, u"dir"_s, dir);
    addParam<IfNotEmpty>(_q, u"recurse"_s, recurse);
    return _q;
}

QUrl GetRelatingEventsJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                          const QString& eventId, const QString& from,
                                          const QString& to, std::optional<int> limit,
                                          const QString& dir, std::optional<bool> recurse)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                                            eventId),
                                   queryToGetRelatingEvents(from, to, limit, dir, recurse));
}

GetRelatingEventsJob::GetRelatingEventsJob(const QString& roomId, const QString& eventId,
                                           const QString& from, const QString& to,
                                           std::optional<int> limit, const QString& dir,
                                           std::optional<bool> recurse)
    : BaseJob(HttpVerb::Get, u"GetRelatingEventsJob"_s,
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/", eventId),
              queryToGetRelatingEvents(from, to, limit, dir, recurse))
{
    addExpectedKey("chunk");
}

auto queryToGetRelatingEventsWithRelType(const QString& from, const QString& to,
                                         std::optional<int> limit, const QString& dir,
                                         std::optional<bool> recurse)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"from"_s, from);
    addParam<IfNotEmpty>(_q, u"to"_s, to);
    addParam<IfNotEmpty>(_q, u"limit"_s, limit);
    addParam<IfNotEmpty>(_q, u"dir"_s, dir);
    addParam<IfNotEmpty>(_q, u"recurse"_s, recurse);
    return _q;
}

QUrl GetRelatingEventsWithRelTypeJob::makeRequestUrl(const HomeserverData& hsData,
                                                     const QString& roomId, const QString& eventId,
                                                     const QString& relType, const QString& from,
                                                     const QString& to, std::optional<int> limit,
                                                     const QString& dir, std::optional<bool> recurse)
{
    return BaseJob::makeRequestUrl(
        hsData,
        makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/", eventId, "/", relType),
        queryToGetRelatingEventsWithRelType(from, to, limit, dir, recurse));
}

GetRelatingEventsWithRelTypeJob::GetRelatingEventsWithRelTypeJob(
    const QString& roomId, const QString& eventId, const QString& relType, const QString& from,
    const QString& to, std::optional<int> limit, const QString& dir, std::optional<bool> recurse)
    : BaseJob(HttpVerb::Get, u"GetRelatingEventsWithRelTypeJob"_s,
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/", eventId, "/",
                       relType),
              queryToGetRelatingEventsWithRelType(from, to, limit, dir, recurse))
{
    addExpectedKey("chunk");
}

auto queryToGetRelatingEventsWithRelTypeAndEventType(const QString& from, const QString& to,
                                                     std::optional<int> limit, const QString& dir,
                                                     std::optional<bool> recurse)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"from"_s, from);
    addParam<IfNotEmpty>(_q, u"to"_s, to);
    addParam<IfNotEmpty>(_q, u"limit"_s, limit);
    addParam<IfNotEmpty>(_q, u"dir"_s, dir);
    addParam<IfNotEmpty>(_q, u"recurse"_s, recurse);
    return _q;
}

QUrl GetRelatingEventsWithRelTypeAndEventTypeJob::makeRequestUrl(
    const HomeserverData& hsData, const QString& roomId, const QString& eventId,
    const QString& relType, const QString& eventType, const QString& from, const QString& to,
    std::optional<int> limit, const QString& dir, std::optional<bool> recurse)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                                            eventId, "/", relType, "/", eventType),
                                   queryToGetRelatingEventsWithRelTypeAndEventType(from, to, limit,
                                                                                   dir, recurse));
}

GetRelatingEventsWithRelTypeAndEventTypeJob::GetRelatingEventsWithRelTypeAndEventTypeJob(
    const QString& roomId, const QString& eventId, const QString& relType, const QString& eventType,
    const QString& from, const QString& to, std::optional<int> limit, const QString& dir,
    std::optional<bool> recurse)
    : BaseJob(HttpVerb::Get, u"GetRelatingEventsWithRelTypeAndEventTypeJob"_s,
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/", eventId, "/",
                       relType, "/", eventType),
              queryToGetRelatingEventsWithRelTypeAndEventType(from, to, limit, dir, recurse))
{
    addExpectedKey("chunk");
}

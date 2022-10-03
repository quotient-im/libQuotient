/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "relations.h"

using namespace Quotient;

auto queryToGetRelatingEvents(const QString& from, const QString& to,
                              Omittable<int> limit, const QString& dir)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("to"), to);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("dir"), dir);
    return _q;
}

QUrl GetRelatingEventsJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                          const QString& eventId,
                                          const QString& from, const QString& to,
                                          Omittable<int> limit,
                                          const QString& dir)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v1", "/rooms/",
                                            roomId, "/relations/", eventId),
                                   queryToGetRelatingEvents(from, to, limit,
                                                            dir));
}

GetRelatingEventsJob::GetRelatingEventsJob(
    const QString& roomId, const QString& eventId, const QString& from,
    const QString& to, Omittable<int> limit, const QString& dir)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRelatingEventsJob"),
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                       eventId),
              queryToGetRelatingEvents(from, to, limit, dir))
{
    addExpectedKey("chunk");
}

auto queryToGetRelatingEventsWithRelType(const QString& from, const QString& to,
                                         Omittable<int> limit,
                                         const QString& dir)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("to"), to);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("dir"), dir);
    return _q;
}

QUrl GetRelatingEventsWithRelTypeJob::makeRequestUrl(
    QUrl baseUrl, const QString& roomId, const QString& eventId,
    const QString& relType, const QString& from, const QString& to,
    Omittable<int> limit, const QString& dir)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl),
        makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                 eventId, "/", relType),
        queryToGetRelatingEventsWithRelType(from, to, limit, dir));
}

GetRelatingEventsWithRelTypeJob::GetRelatingEventsWithRelTypeJob(
    const QString& roomId, const QString& eventId, const QString& relType,
    const QString& from, const QString& to, Omittable<int> limit,
    const QString& dir)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRelatingEventsWithRelTypeJob"),
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                       eventId, "/", relType),
              queryToGetRelatingEventsWithRelType(from, to, limit, dir))
{
    addExpectedKey("chunk");
}

auto queryToGetRelatingEventsWithRelTypeAndEventType(const QString& from,
                                                     const QString& to,
                                                     Omittable<int> limit,
                                                     const QString& dir)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("to"), to);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("dir"), dir);
    return _q;
}

QUrl GetRelatingEventsWithRelTypeAndEventTypeJob::makeRequestUrl(
    QUrl baseUrl, const QString& roomId, const QString& eventId,
    const QString& relType, const QString& eventType, const QString& from,
    const QString& to, Omittable<int> limit, const QString& dir)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl),
        makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                 eventId, "/", relType, "/", eventType),
        queryToGetRelatingEventsWithRelTypeAndEventType(from, to, limit, dir));
}

GetRelatingEventsWithRelTypeAndEventTypeJob::
    GetRelatingEventsWithRelTypeAndEventTypeJob(
        const QString& roomId, const QString& eventId, const QString& relType,
        const QString& eventType, const QString& from, const QString& to,
        Omittable<int> limit, const QString& dir)
    : BaseJob(HttpVerb::Get,
              QStringLiteral("GetRelatingEventsWithRelTypeAndEventTypeJob"),
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                       eventId, "/", relType, "/", eventType),
              queryToGetRelatingEventsWithRelTypeAndEventType(from, to, limit,
                                                              dir))
{
    addExpectedKey("chunk");
}

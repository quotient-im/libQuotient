// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "relations.h"

using namespace Quotient;

auto queryToGetRelatingEvents(const QString& from, const QString& to, std::optional<int> limit,
                              const QString& dir, std::optional<bool> recurse)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("to"), to);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("dir"), dir);
    addParam<IfNotEmpty>(_q, QStringLiteral("recurse"), recurse);
    return _q;
}

QUrl GetRelatingEventsJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                          const QString& eventId, const QString& from,
                                          const QString& to, std::optional<int> limit,
                                          const QString& dir, std::optional<bool> recurse)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                                            eventId),
                                   queryToGetRelatingEvents(from, to, limit, dir, recurse));
}

GetRelatingEventsJob::GetRelatingEventsJob(const QString& roomId, const QString& eventId,
                                           const QString& from, const QString& to,
                                           std::optional<int> limit, const QString& dir,
                                           std::optional<bool> recurse)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRelatingEventsJob"),
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
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("to"), to);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("dir"), dir);
    addParam<IfNotEmpty>(_q, QStringLiteral("recurse"), recurse);
    return _q;
}

QUrl GetRelatingEventsWithRelTypeJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                                     const QString& eventId, const QString& relType,
                                                     const QString& from, const QString& to,
                                                     std::optional<int> limit, const QString& dir,
                                                     std::optional<bool> recurse)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl),
        makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/", eventId, "/", relType),
        queryToGetRelatingEventsWithRelType(from, to, limit, dir, recurse));
}

GetRelatingEventsWithRelTypeJob::GetRelatingEventsWithRelTypeJob(
    const QString& roomId, const QString& eventId, const QString& relType, const QString& from,
    const QString& to, std::optional<int> limit, const QString& dir, std::optional<bool> recurse)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRelatingEventsWithRelTypeJob"),
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
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    addParam<IfNotEmpty>(_q, QStringLiteral("to"), to);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("dir"), dir);
    addParam<IfNotEmpty>(_q, QStringLiteral("recurse"), recurse);
    return _q;
}

QUrl GetRelatingEventsWithRelTypeAndEventTypeJob::makeRequestUrl(
    QUrl baseUrl, const QString& roomId, const QString& eventId, const QString& relType,
    const QString& eventType, const QString& from, const QString& to, std::optional<int> limit,
    const QString& dir, std::optional<bool> recurse)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/",
                                            eventId, "/", relType, "/", eventType),
                                   queryToGetRelatingEventsWithRelTypeAndEventType(from, to, limit,
                                                                                   dir, recurse));
}

GetRelatingEventsWithRelTypeAndEventTypeJob::GetRelatingEventsWithRelTypeAndEventTypeJob(
    const QString& roomId, const QString& eventId, const QString& relType, const QString& eventType,
    const QString& from, const QString& to, std::optional<int> limit, const QString& dir,
    std::optional<bool> recurse)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetRelatingEventsWithRelTypeAndEventTypeJob"),
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/relations/", eventId, "/",
                       relType, "/", eventType),
              queryToGetRelatingEventsWithRelTypeAndEventType(from, to, limit, dir, recurse))
{
    addExpectedKey("chunk");
}

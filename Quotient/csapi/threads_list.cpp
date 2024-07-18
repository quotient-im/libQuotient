// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "threads_list.h"

using namespace Quotient;

auto queryToGetThreadRoots(const QString& include, std::optional<int> limit, const QString& from)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("include"), include);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    return _q;
}

QUrl GetThreadRootsJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                       const QString& include, std::optional<int> limit,
                                       const QString& from)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v1", "/rooms/", roomId, "/threads"),
                                   queryToGetThreadRoots(include, limit, from));
}

GetThreadRootsJob::GetThreadRootsJob(const QString& roomId, const QString& include,
                                     std::optional<int> limit, const QString& from)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetThreadRootsJob"),
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/threads"),
              queryToGetThreadRoots(include, limit, from))
{
    addExpectedKey("chunk");
}

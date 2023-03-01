/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "space_hierarchy.h"

using namespace Quotient;

auto queryToGetSpaceHierarchy(Omittable<bool> suggestedOnly,
                              Omittable<int> limit, Omittable<int> maxDepth,
                              const QString& from)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("suggested_only"), suggestedOnly);
    addParam<IfNotEmpty>(_q, QStringLiteral("limit"), limit);
    addParam<IfNotEmpty>(_q, QStringLiteral("max_depth"), maxDepth);
    addParam<IfNotEmpty>(_q, QStringLiteral("from"), from);
    return _q;
}

QUrl GetSpaceHierarchyJob::makeRequestUrl(QUrl baseUrl, const QString& roomId,
                                          Omittable<bool> suggestedOnly,
                                          Omittable<int> limit,
                                          Omittable<int> maxDepth,
                                          const QString& from)
{
    return BaseJob::makeRequestUrl(
        std::move(baseUrl),
        makePath("/_matrix/client/v1", "/rooms/", roomId, "/hierarchy"),
        queryToGetSpaceHierarchy(suggestedOnly, limit, maxDepth, from));
}

GetSpaceHierarchyJob::GetSpaceHierarchyJob(const QString& roomId,
                                           Omittable<bool> suggestedOnly,
                                           Omittable<int> limit,
                                           Omittable<int> maxDepth,
                                           const QString& from)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetSpaceHierarchyJob"),
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/hierarchy"),
              queryToGetSpaceHierarchy(suggestedOnly, limit, maxDepth, from))
{
    addExpectedKey("rooms");
}

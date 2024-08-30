// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "space_hierarchy.h"

using namespace Quotient;

auto queryToGetSpaceHierarchy(std::optional<bool> suggestedOnly, std::optional<int> limit,
                              std::optional<int> maxDepth, const QString& from)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"suggested_only"_s, suggestedOnly);
    addParam<IfNotEmpty>(_q, u"limit"_s, limit);
    addParam<IfNotEmpty>(_q, u"max_depth"_s, maxDepth);
    addParam<IfNotEmpty>(_q, u"from"_s, from);
    return _q;
}

QUrl GetSpaceHierarchyJob::makeRequestUrl(const HomeserverData& hsData, const QString& roomId,
                                          std::optional<bool> suggestedOnly,
                                          std::optional<int> limit, std::optional<int> maxDepth,
                                          const QString& from)
{
    return BaseJob::makeRequestUrl(hsData,
                                   makePath("/_matrix/client/v1", "/rooms/", roomId, "/hierarchy"),
                                   queryToGetSpaceHierarchy(suggestedOnly, limit, maxDepth, from));
}

GetSpaceHierarchyJob::GetSpaceHierarchyJob(const QString& roomId, std::optional<bool> suggestedOnly,
                                           std::optional<int> limit, std::optional<int> maxDepth,
                                           const QString& from)
    : BaseJob(HttpVerb::Get, u"GetSpaceHierarchyJob"_s,
              makePath("/_matrix/client/v1", "/rooms/", roomId, "/hierarchy"),
              queryToGetSpaceHierarchy(suggestedOnly, limit, maxDepth, from))
{
    addExpectedKey("rooms");
}

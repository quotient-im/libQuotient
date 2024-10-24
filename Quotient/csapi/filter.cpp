// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "filter.h"

using namespace Quotient;

DefineFilterJob::DefineFilterJob(const QString& userId, const Filter& filter)
    : BaseJob(HttpVerb::Post, u"DefineFilterJob"_s,
              makePath("/_matrix/client/v3", "/user/", userId, "/filter"))
{
    setRequestData({ toJson(filter) });
    addExpectedKey(u"filter_id"_s);
}

QUrl GetFilterJob::makeRequestUrl(const HomeserverData& hsData, const QString& userId,
                                  const QString& filterId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/user/", userId,
                                                    "/filter/", filterId));
}

GetFilterJob::GetFilterJob(const QString& userId, const QString& filterId)
    : BaseJob(HttpVerb::Get, u"GetFilterJob"_s,
              makePath("/_matrix/client/v3", "/user/", userId, "/filter/", filterId))
{}

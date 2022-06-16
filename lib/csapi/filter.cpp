/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "filter.h"

using namespace Quotient;

DefineFilterJob::DefineFilterJob(const QString& userId, const Filter& filter)
    : BaseJob(HttpVerb::Post, QStringLiteral("DefineFilterJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/filter"))
{
    setRequestData({ toJson(filter) });
    addExpectedKey("filter_id");
}

QUrl GetFilterJob::makeRequestUrl(QUrl baseUrl, const QString& userId,
                                  const QString& filterId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/user/",
                                            userId, "/filter/", filterId));
}

GetFilterJob::GetFilterJob(const QString& userId, const QString& filterId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetFilterJob"),
              makePath("/_matrix/client/v3", "/user/", userId, "/filter/",
                       filterId))
{}

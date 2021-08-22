/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "filter.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

DefineFilterJob::DefineFilterJob(const QString& userId, const Filter& filter)
    : BaseJob(HttpVerb::Post, QStringLiteral("DefineFilterJob"),
              QStringLiteral("/_matrix/client/r0") % "/user/" % userId
                  % "/filter")
{
    setRequestData(RequestData(toJson(filter)));
    addExpectedKey("filter_id");
}

QUrl GetFilterJob::makeRequestUrl(QUrl baseUrl, const QString& userId,
                                  const QString& filterId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0") % "/user/"
                                       % userId % "/filter/" % filterId);
}

GetFilterJob::GetFilterJob(const QString& userId, const QString& filterId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetFilterJob"),
              QStringLiteral("/_matrix/client/r0") % "/user/" % userId
                  % "/filter/" % filterId)
{}

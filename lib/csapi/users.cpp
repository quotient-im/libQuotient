/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "users.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

SearchUserDirectoryJob::SearchUserDirectoryJob(const QString& searchTerm,
                                               Omittable<int> limit)
    : BaseJob(HttpVerb::Post, QStringLiteral("SearchUserDirectoryJob"),
              QStringLiteral("/_matrix/client/r0") % "/user_directory/search")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("search_term"), searchTerm);
    addParam<IfNotEmpty>(_data, QStringLiteral("limit"), limit);
    setRequestData(std::move(_data));
    addExpectedKey("results");
    addExpectedKey("limited");
}

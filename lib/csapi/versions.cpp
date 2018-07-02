/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "versions.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client");

class GetVersionsJob::Private
{
    public:
        QStringList versions;
};

QUrl GetVersionsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/versions");
}

static const auto GetVersionsJobName = QStringLiteral("GetVersionsJob");

GetVersionsJob::GetVersionsJob()
    : BaseJob(HttpVerb::Get, GetVersionsJobName,
        basePath % "/versions", false)
    , d(new Private)
{
}

GetVersionsJob::~GetVersionsJob() = default;

const QStringList& GetVersionsJob::versions() const
{
    return d->versions;
}

BaseJob::Status GetVersionsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->versions = fromJson<QStringList>(json.value("versions"_ls));
    return Success;
}


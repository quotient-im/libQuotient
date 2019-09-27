/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "versions.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client");

class GetVersionsJob::Private
{
public:
    QStringList versions;
    QHash<QString, bool> unstableFeatures;
};

QUrl GetVersionsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/versions");
}

static const auto GetVersionsJobName = QStringLiteral("GetVersionsJob");

GetVersionsJob::GetVersionsJob()
    : BaseJob(HttpVerb::Get, GetVersionsJobName, basePath % "/versions", false)
    , d(new Private)
{}

GetVersionsJob::~GetVersionsJob() = default;

const QStringList& GetVersionsJob::versions() const { return d->versions; }

const QHash<QString, bool>& GetVersionsJob::unstableFeatures() const
{
    return d->unstableFeatures;
}

BaseJob::Status GetVersionsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("versions"_ls))
        return { IncorrectResponse,
                 "The key 'versions' not found in the response" };
    fromJson(json.value("versions"_ls), d->versions);
    fromJson(json.value("unstable_features"_ls), d->unstableFeatures);

    return Success;
}

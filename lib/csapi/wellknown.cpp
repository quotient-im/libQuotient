/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "wellknown.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/.well-known");

class GetWellknownJob::Private
{
    public:
        DiscoveryInformation data;
};

QUrl GetWellknownJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/matrix/client");
}

static const auto GetWellknownJobName = QStringLiteral("GetWellknownJob");

GetWellknownJob::GetWellknownJob()
    : BaseJob(HttpVerb::Get, GetWellknownJobName,
        basePath % "/matrix/client", false)
    , d(new Private)
{
}

GetWellknownJob::~GetWellknownJob() = default;

const DiscoveryInformation& GetWellknownJob::data() const
{
    return d->data;
}

BaseJob::Status GetWellknownJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);
    return Success;
}


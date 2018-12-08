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
        HomeserverInformation homeserver;
        Omittable<IdentityServerInformation> identityServer;
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

const HomeserverInformation& GetWellknownJob::homeserver() const
{
    return d->homeserver;
}

const Omittable<IdentityServerInformation>& GetWellknownJob::identityServer() const
{
    return d->identityServer;
}

BaseJob::Status GetWellknownJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("m.homeserver"_ls))
        return { JsonParseError,
            "The key 'm.homeserver' not found in the response" };
    fromJson(json.value("m.homeserver"_ls), d->homeserver);
    fromJson(json.value("m.identity_server"_ls), d->identityServer);
    return Success;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "capabilities.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

// Converters
namespace QMatrixClient
{

template <>
struct JsonObjectConverter<GetCapabilitiesJob::ChangePasswordCapability>
{
    static void fillFrom(const QJsonObject& jo,
                         GetCapabilitiesJob::ChangePasswordCapability& result)
    {
        fromJson(jo.value("enabled"_ls), result.enabled);
    }
};

template <>
struct JsonObjectConverter<GetCapabilitiesJob::RoomVersionsCapability>
{
    static void fillFrom(const QJsonObject& jo,
                         GetCapabilitiesJob::RoomVersionsCapability& result)
    {
        fromJson(jo.value("default"_ls), result.defaultVersion);
        fromJson(jo.value("available"_ls), result.available);
    }
};

template <>
struct JsonObjectConverter<GetCapabilitiesJob::Capabilities>
{
    static void fillFrom(QJsonObject jo,
                         GetCapabilitiesJob::Capabilities& result)
    {
        fromJson(jo.take("m.change_password"_ls), result.changePassword);
        fromJson(jo.take("m.room_versions"_ls), result.roomVersions);
        fromJson(jo, result.additionalProperties);
    }
};

} // namespace QMatrixClient

class GetCapabilitiesJob::Private
{
public:
    Capabilities capabilities;
};

QUrl GetCapabilitiesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/capabilities");
}

static const auto GetCapabilitiesJobName = QStringLiteral("GetCapabilitiesJob");

GetCapabilitiesJob::GetCapabilitiesJob()
    : BaseJob(HttpVerb::Get, GetCapabilitiesJobName, basePath % "/capabilities")
    , d(new Private)
{}

GetCapabilitiesJob::~GetCapabilitiesJob() = default;

const GetCapabilitiesJob::Capabilities& GetCapabilitiesJob::capabilities() const
{
    return d->capabilities;
}

BaseJob::Status GetCapabilitiesJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("capabilities"_ls))
        return { IncorrectResponse,
                 "The key 'capabilities' not found in the response" };
    fromJson(json.value("capabilities"_ls), d->capabilities);

    return Success;
}

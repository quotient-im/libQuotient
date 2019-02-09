/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "capabilities.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    template <> struct JsonObjectConverter<GetCapabilitiesJob::ChangePasswordCapability>
    {
        static void fillFrom(const QJsonObject& jo, GetCapabilitiesJob::ChangePasswordCapability& result)
        {
            fromJson(jo.value("enabled"_ls), result.enabled);
        }
    };

    template <> struct JsonObjectConverter<GetCapabilitiesJob::RoomVersionsCapability>
    {
        static void fillFrom(const QJsonObject& jo, GetCapabilitiesJob::RoomVersionsCapability& result)
        {
            fromJson(jo.value("default"_ls), result.isDefault);
            fromJson(jo.value("available"_ls), result.available);
        }
    };
} // namespace QMatrixClient

class GetCapabilitiesJob::Private
{
    public:
        Omittable<ChangePasswordCapability> changePassword;
        Omittable<RoomVersionsCapability> roomVersions;
};

QUrl GetCapabilitiesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/capabilities");
}

static const auto GetCapabilitiesJobName = QStringLiteral("GetCapabilitiesJob");

GetCapabilitiesJob::GetCapabilitiesJob()
    : BaseJob(HttpVerb::Get, GetCapabilitiesJobName,
        basePath % "/capabilities")
    , d(new Private)
{
}

GetCapabilitiesJob::~GetCapabilitiesJob() = default;

const Omittable<GetCapabilitiesJob::ChangePasswordCapability>& GetCapabilitiesJob::changePassword() const
{
    return d->changePassword;
}

const Omittable<GetCapabilitiesJob::RoomVersionsCapability>& GetCapabilitiesJob::roomVersions() const
{
    return d->roomVersions;
}

BaseJob::Status GetCapabilitiesJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("m.change_password"_ls), d->changePassword);
    fromJson(json.value("m.room_versions"_ls), d->roomVersions);
    return Success;
}


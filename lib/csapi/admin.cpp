/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "admin.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

    template <> struct JsonObjectConverter<GetWhoIsJob::ConnectionInfo>
    {
        static void fillFrom(const QJsonObject& jo, GetWhoIsJob::ConnectionInfo& result)
        {
            fromJson(jo.value("ip"_ls), result.ip);
            fromJson(jo.value("last_seen"_ls), result.lastSeen);
            fromJson(jo.value("user_agent"_ls), result.userAgent);
        }
    };

    template <> struct JsonObjectConverter<GetWhoIsJob::SessionInfo>
    {
        static void fillFrom(const QJsonObject& jo, GetWhoIsJob::SessionInfo& result)
        {
            fromJson(jo.value("connections"_ls), result.connections);
        }
    };

    template <> struct JsonObjectConverter<GetWhoIsJob::DeviceInfo>
    {
        static void fillFrom(const QJsonObject& jo, GetWhoIsJob::DeviceInfo& result)
        {
            fromJson(jo.value("sessions"_ls), result.sessions);
        }
    };
} // namespace QMatrixClient

class GetWhoIsJob::Private
{
    public:
        QString userId;
        QHash<QString, DeviceInfo> devices;
};

QUrl GetWhoIsJob::makeRequestUrl(QUrl baseUrl, const QString& userId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/admin/whois/" % userId);
}

static const auto GetWhoIsJobName = QStringLiteral("GetWhoIsJob");

GetWhoIsJob::GetWhoIsJob(const QString& userId)
    : BaseJob(HttpVerb::Get, GetWhoIsJobName,
        basePath % "/admin/whois/" % userId)
    , d(new Private)
{
}

GetWhoIsJob::~GetWhoIsJob() = default;

const QString& GetWhoIsJob::userId() const
{
    return d->userId;
}

const QHash<QString, GetWhoIsJob::DeviceInfo>& GetWhoIsJob::devices() const
{
    return d->devices;
}

BaseJob::Status GetWhoIsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("user_id"_ls), d->userId);
    fromJson(json.value("devices"_ls), d->devices);
    return Success;
}


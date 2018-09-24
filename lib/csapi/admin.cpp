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

    template <> struct FromJsonObject<GetWhoIsJob::ConnectionInfo>
    {
        GetWhoIsJob::ConnectionInfo operator()(const QJsonObject& jo) const
        {
            GetWhoIsJob::ConnectionInfo result;
            result.ip =
                fromJson<QString>(jo.value("ip"_ls));
            result.lastSeen =
                fromJson<qint64>(jo.value("last_seen"_ls));
            result.userAgent =
                fromJson<QString>(jo.value("user_agent"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<GetWhoIsJob::SessionInfo>
    {
        GetWhoIsJob::SessionInfo operator()(const QJsonObject& jo) const
        {
            GetWhoIsJob::SessionInfo result;
            result.connections =
                fromJson<QVector<GetWhoIsJob::ConnectionInfo>>(jo.value("connections"_ls));

            return result;
        }
    };

    template <> struct FromJsonObject<GetWhoIsJob::DeviceInfo>
    {
        GetWhoIsJob::DeviceInfo operator()(const QJsonObject& jo) const
        {
            GetWhoIsJob::DeviceInfo result;
            result.sessions =
                fromJson<QVector<GetWhoIsJob::SessionInfo>>(jo.value("sessions"_ls));

            return result;
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
    d->userId = fromJson<QString>(json.value("user_id"_ls));
    d->devices = fromJson<QHash<QString, DeviceInfo>>(json.value("devices"_ls));
    return Success;
}


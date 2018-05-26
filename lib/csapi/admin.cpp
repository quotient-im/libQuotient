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

    template <> struct FromJson<GetWhoIsJob::ConnectionInfo>
    {
        GetWhoIsJob::ConnectionInfo operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            GetWhoIsJob::ConnectionInfo result;
            result.ip =
                fromJson<QString>(_json.value("ip"));
            result.lastSeen =
                fromJson<qint64>(_json.value("last_seen"));
            result.userAgent =
                fromJson<QString>(_json.value("user_agent"));

            return result;
        }
    };

    template <> struct FromJson<GetWhoIsJob::SessionInfo>
    {
        GetWhoIsJob::SessionInfo operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            GetWhoIsJob::SessionInfo result;
            result.connections =
                fromJson<QVector<GetWhoIsJob::ConnectionInfo>>(_json.value("connections"));

            return result;
        }
    };

    template <> struct FromJson<GetWhoIsJob::DeviceInfo>
    {
        GetWhoIsJob::DeviceInfo operator()(const QJsonValue& jv)
        {
            const auto& _json = jv.toObject();
            GetWhoIsJob::DeviceInfo result;
            result.sessions =
                fromJson<QVector<GetWhoIsJob::SessionInfo>>(_json.value("sessions"));

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

GetWhoIsJob::GetWhoIsJob(const QString& userId)
    : BaseJob(HttpVerb::Get, "GetWhoIsJob",
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
    d->userId = fromJson<QString>(json.value("user_id"));
    d->devices = fromJson<QHash<QString, DeviceInfo>>(json.value("devices"));
    return Success;
}


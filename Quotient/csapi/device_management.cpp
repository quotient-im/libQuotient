// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "device_management.h"

using namespace Quotient;

QUrl GetDevicesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/_matrix/client/v3", "/devices"));
}

GetDevicesJob::GetDevicesJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetDevicesJob"),
              makePath("/_matrix/client/v3", "/devices"))
{}

QUrl GetDeviceJob::makeRequestUrl(QUrl baseUrl, const QString& deviceId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/devices/", deviceId));
}

GetDeviceJob::GetDeviceJob(const QString& deviceId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetDeviceJob"),
              makePath("/_matrix/client/v3", "/devices/", deviceId))
{}

UpdateDeviceJob::UpdateDeviceJob(const QString& deviceId, const QString& displayName)
    : BaseJob(HttpVerb::Put, QStringLiteral("UpdateDeviceJob"),
              makePath("/_matrix/client/v3", "/devices/", deviceId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("display_name"), displayName);
    setRequestData({ _dataJson });
}

DeleteDeviceJob::DeleteDeviceJob(const QString& deviceId, const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteDeviceJob"),
              makePath("/_matrix/client/v3", "/devices/", deviceId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("auth"), auth);
    setRequestData({ _dataJson });
}

DeleteDevicesJob::DeleteDevicesJob(const QStringList& devices,
                                   const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("DeleteDevicesJob"),
              makePath("/_matrix/client/v3", "/delete_devices"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("devices"), devices);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("auth"), auth);
    setRequestData({ _dataJson });
}

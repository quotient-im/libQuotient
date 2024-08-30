// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "device_management.h"

using namespace Quotient;

QUrl GetDevicesJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/devices"));
}

GetDevicesJob::GetDevicesJob()
    : BaseJob(HttpVerb::Get, u"GetDevicesJob"_s, makePath("/_matrix/client/v3", "/devices"))
{}

QUrl GetDeviceJob::makeRequestUrl(const HomeserverData& hsData, const QString& deviceId)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/devices/", deviceId));
}

GetDeviceJob::GetDeviceJob(const QString& deviceId)
    : BaseJob(HttpVerb::Get, u"GetDeviceJob"_s,
              makePath("/_matrix/client/v3", "/devices/", deviceId))
{}

UpdateDeviceJob::UpdateDeviceJob(const QString& deviceId, const QString& displayName)
    : BaseJob(HttpVerb::Put, u"UpdateDeviceJob"_s,
              makePath("/_matrix/client/v3", "/devices/", deviceId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "display_name"_L1, displayName);
    setRequestData({ _dataJson });
}

DeleteDeviceJob::DeleteDeviceJob(const QString& deviceId,
                                 const std::optional<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Delete, u"DeleteDeviceJob"_s,
              makePath("/_matrix/client/v3", "/devices/", deviceId))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "auth"_L1, auth);
    setRequestData({ _dataJson });
}

DeleteDevicesJob::DeleteDevicesJob(const QStringList& devices,
                                   const std::optional<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, u"DeleteDevicesJob"_s,
              makePath("/_matrix/client/v3", "/delete_devices"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "devices"_L1, devices);
    addParam<IfNotEmpty>(_dataJson, "auth"_L1, auth);
    setRequestData({ _dataJson });
}

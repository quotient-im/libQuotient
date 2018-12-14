/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "device_management.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetDevicesJob::Private
{
    public:
        QVector<Device> devices;
};

QUrl GetDevicesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/devices");
}

static const auto GetDevicesJobName = QStringLiteral("GetDevicesJob");

GetDevicesJob::GetDevicesJob()
    : BaseJob(HttpVerb::Get, GetDevicesJobName,
        basePath % "/devices")
    , d(new Private)
{
}

GetDevicesJob::~GetDevicesJob() = default;

const QVector<Device>& GetDevicesJob::devices() const
{
    return d->devices;
}

BaseJob::Status GetDevicesJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("devices"_ls), d->devices);
    return Success;
}

class GetDeviceJob::Private
{
    public:
        Device data;
};

QUrl GetDeviceJob::makeRequestUrl(QUrl baseUrl, const QString& deviceId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/devices/" % deviceId);
}

static const auto GetDeviceJobName = QStringLiteral("GetDeviceJob");

GetDeviceJob::GetDeviceJob(const QString& deviceId)
    : BaseJob(HttpVerb::Get, GetDeviceJobName,
        basePath % "/devices/" % deviceId)
    , d(new Private)
{
}

GetDeviceJob::~GetDeviceJob() = default;

const Device& GetDeviceJob::data() const
{
    return d->data;
}

BaseJob::Status GetDeviceJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);
    return Success;
}

static const auto UpdateDeviceJobName = QStringLiteral("UpdateDeviceJob");

UpdateDeviceJob::UpdateDeviceJob(const QString& deviceId, const QString& displayName)
    : BaseJob(HttpVerb::Put, UpdateDeviceJobName,
        basePath % "/devices/" % deviceId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("display_name"), displayName);
    setRequestData(_data);
}

static const auto DeleteDeviceJobName = QStringLiteral("DeleteDeviceJob");

DeleteDeviceJob::DeleteDeviceJob(const QString& deviceId, const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Delete, DeleteDeviceJobName,
        basePath % "/devices/" % deviceId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(_data);
}

static const auto DeleteDevicesJobName = QStringLiteral("DeleteDevicesJob");

DeleteDevicesJob::DeleteDevicesJob(const QStringList& devices, const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, DeleteDevicesJobName,
        basePath % "/delete_devices")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("devices"), devices);
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(_data);
}


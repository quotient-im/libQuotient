/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "device_management.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class GetDevicesJob::Private {
public:
    QVector<Device> devices;
};

QUrl GetDevicesJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/devices");
}

GetDevicesJob::GetDevicesJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetDevicesJob"),
              basePath % "/devices")
    , d(new Private)
{}

GetDevicesJob::~GetDevicesJob() = default;

const QVector<Device>& GetDevicesJob::devices() const { return d->devices; }

BaseJob::Status GetDevicesJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("devices"_ls), d->devices);

    return Success;
}

class GetDeviceJob::Private {
public:
    Device data;
};

QUrl GetDeviceJob::makeRequestUrl(QUrl baseUrl, const QString& deviceId)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/devices/" % deviceId);
}

GetDeviceJob::GetDeviceJob(const QString& deviceId)
    : BaseJob(HttpVerb::Get, QStringLiteral("GetDeviceJob"),
              basePath % "/devices/" % deviceId)
    , d(new Private)
{}

GetDeviceJob::~GetDeviceJob() = default;

const Device& GetDeviceJob::data() const { return d->data; }

BaseJob::Status GetDeviceJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

UpdateDeviceJob::UpdateDeviceJob(const QString& deviceId,
                                 const QString& displayName)
    : BaseJob(HttpVerb::Put, QStringLiteral("UpdateDeviceJob"),
              basePath % "/devices/" % deviceId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("display_name"), displayName);
    setRequestData(_data);
}

DeleteDeviceJob::DeleteDeviceJob(const QString& deviceId,
                                 const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Delete, QStringLiteral("DeleteDeviceJob"),
              basePath % "/devices/" % deviceId)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(_data);
}

DeleteDevicesJob::DeleteDevicesJob(const QStringList& devices,
                                   const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("DeleteDevicesJob"),
              basePath % "/delete_devices")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("devices"), devices);
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(_data);
}

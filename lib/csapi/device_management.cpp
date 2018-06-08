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

GetDevicesJob::GetDevicesJob()
    : BaseJob(HttpVerb::Get, "GetDevicesJob",
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
    d->devices = fromJson<QVector<Device>>(json.value("devices"));
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

GetDeviceJob::GetDeviceJob(const QString& deviceId)
    : BaseJob(HttpVerb::Get, "GetDeviceJob",
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
    auto json = data.object();
    if (!json.contains("data"))
        return { JsonParseError,
            "The key 'data' not found in the response" };
    d->data = fromJson<Device>(json.value("data"));
    return Success;
}

UpdateDeviceJob::UpdateDeviceJob(const QString& deviceId, const QString& displayName)
    : BaseJob(HttpVerb::Put, "UpdateDeviceJob",
        basePath % "/devices/" % deviceId)
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "display_name", displayName);
    setRequestData(_data);
}

DeleteDeviceJob::DeleteDeviceJob(const QString& deviceId, const QJsonObject& auth)
    : BaseJob(HttpVerb::Delete, "DeleteDeviceJob",
        basePath % "/devices/" % deviceId)
{
    QJsonObject _data;
    addToJson<IfNotEmpty>(_data, "auth", auth);
    setRequestData(_data);
}

DeleteDevicesJob::DeleteDevicesJob(const QStringList& devices, const QJsonObject& auth)
    : BaseJob(HttpVerb::Post, "DeleteDevicesJob",
        basePath % "/delete_devices")
{
    QJsonObject _data;
    addToJson<>(_data, "devices", devices);
    addToJson<IfNotEmpty>(_data, "auth", auth);
    setRequestData(_data);
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "login.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class LoginJob::Private
{
    public:
        QString userId;
        QString accessToken;
        QString homeServer;
        QString deviceId;
};

LoginJob::LoginJob(const QString& type, const QString& user, const QString& medium, const QString& address, const QString& password, const QString& token, const QString& deviceId, const QString& initialDeviceDisplayName)
    : BaseJob(HttpVerb::Post, "LoginJob",
        basePath % "/login", false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, "type", type);
    addParam<IfNotEmpty>(_data, "user", user);
    addParam<IfNotEmpty>(_data, "medium", medium);
    addParam<IfNotEmpty>(_data, "address", address);
    addParam<IfNotEmpty>(_data, "password", password);
    addParam<IfNotEmpty>(_data, "token", token);
    addParam<IfNotEmpty>(_data, "device_id", deviceId);
    addParam<IfNotEmpty>(_data, "initial_device_display_name", initialDeviceDisplayName);
    setRequestData(_data);
}

LoginJob::~LoginJob() = default;

const QString& LoginJob::userId() const
{
    return d->userId;
}

const QString& LoginJob::accessToken() const
{
    return d->accessToken;
}

const QString& LoginJob::homeServer() const
{
    return d->homeServer;
}

const QString& LoginJob::deviceId() const
{
    return d->deviceId;
}

BaseJob::Status LoginJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->userId = fromJson<QString>(json.value("user_id"));
    d->accessToken = fromJson<QString>(json.value("access_token"));
    d->homeServer = fromJson<QString>(json.value("home_server"));
    d->deviceId = fromJson<QString>(json.value("device_id"));
    return Success;
}


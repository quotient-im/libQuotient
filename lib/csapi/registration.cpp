/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "registration.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class RegisterJob::Private
{
    public:
        QString userId;
        QString accessToken;
        QString homeServer;
        QString deviceId;
};

BaseJob::Query queryToRegister(const QString& kind)
{
    BaseJob::Query _q;
    addParam<IfNotEmpty>(_q, "kind", kind);
    return _q;
}

RegisterJob::RegisterJob(const QString& kind, const QJsonObject& auth, bool bindEmail, const QString& username, const QString& password, const QString& deviceId, const QString& initialDeviceDisplayName)
    : BaseJob(HttpVerb::Post, "RegisterJob",
        basePath % "/register",
        queryToRegister(kind),
        {}, false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, "auth", auth);
    addParam<IfNotEmpty>(_data, "bind_email", bindEmail);
    addParam<IfNotEmpty>(_data, "username", username);
    addParam<IfNotEmpty>(_data, "password", password);
    addParam<IfNotEmpty>(_data, "device_id", deviceId);
    addParam<IfNotEmpty>(_data, "initial_device_display_name", initialDeviceDisplayName);
    setRequestData(_data);
}

RegisterJob::~RegisterJob() = default;

const QString& RegisterJob::userId() const
{
    return d->userId;
}

const QString& RegisterJob::accessToken() const
{
    return d->accessToken;
}

const QString& RegisterJob::homeServer() const
{
    return d->homeServer;
}

const QString& RegisterJob::deviceId() const
{
    return d->deviceId;
}

BaseJob::Status RegisterJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->userId = fromJson<QString>(json.value("user_id"));
    d->accessToken = fromJson<QString>(json.value("access_token"));
    d->homeServer = fromJson<QString>(json.value("home_server"));
    d->deviceId = fromJson<QString>(json.value("device_id"));
    return Success;
}

RequestTokenToRegisterJob::RequestTokenToRegisterJob(const QString& clientSecret, const QString& email, int sendAttempt, const QString& idServer)
    : BaseJob(HttpVerb::Post, "RequestTokenToRegisterJob",
        basePath % "/register/email/requestToken", false)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, "id_server", idServer);
    addParam<>(_data, "client_secret", clientSecret);
    addParam<>(_data, "email", email);
    addParam<>(_data, "send_attempt", sendAttempt);
    setRequestData(_data);
}

ChangePasswordJob::ChangePasswordJob(const QString& newPassword, const QJsonObject& auth)
    : BaseJob(HttpVerb::Post, "ChangePasswordJob",
        basePath % "/account/password")
{
    QJsonObject _data;
    addParam<>(_data, "new_password", newPassword);
    addParam<IfNotEmpty>(_data, "auth", auth);
    setRequestData(_data);
}

QUrl RequestTokenToResetPasswordJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/account/password/email/requestToken");
}

RequestTokenToResetPasswordJob::RequestTokenToResetPasswordJob()
    : BaseJob(HttpVerb::Post, "RequestTokenToResetPasswordJob",
        basePath % "/account/password/email/requestToken", false)
{
}

DeactivateAccountJob::DeactivateAccountJob(const QJsonObject& auth)
    : BaseJob(HttpVerb::Post, "DeactivateAccountJob",
        basePath % "/account/deactivate")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, "auth", auth);
    setRequestData(_data);
}

class CheckUsernameAvailabilityJob::Private
{
    public:
        bool available;
};

BaseJob::Query queryToCheckUsernameAvailability(const QString& username)
{
    BaseJob::Query _q;
    addParam<>(_q, "username", username);
    return _q;
}

QUrl CheckUsernameAvailabilityJob::makeRequestUrl(QUrl baseUrl, const QString& username)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/register/available",
            queryToCheckUsernameAvailability(username));
}

CheckUsernameAvailabilityJob::CheckUsernameAvailabilityJob(const QString& username)
    : BaseJob(HttpVerb::Get, "CheckUsernameAvailabilityJob",
        basePath % "/register/available",
        queryToCheckUsernameAvailability(username),
        {}, false)
    , d(new Private)
{
}

CheckUsernameAvailabilityJob::~CheckUsernameAvailabilityJob() = default;

bool CheckUsernameAvailabilityJob::available() const
{
    return d->available;
}

BaseJob::Status CheckUsernameAvailabilityJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->available = fromJson<bool>(json.value("available"));
    return Success;
}


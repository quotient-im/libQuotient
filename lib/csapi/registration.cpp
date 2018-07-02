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
    addParam<IfNotEmpty>(_q, QStringLiteral("kind"), kind);
    return _q;
}

static const auto RegisterJobName = QStringLiteral("RegisterJob");

RegisterJob::RegisterJob(const QString& kind, const QJsonObject& auth, bool bindEmail, const QString& username, const QString& password, const QString& deviceId, const QString& initialDeviceDisplayName)
    : BaseJob(HttpVerb::Post, RegisterJobName,
        basePath % "/register",
        queryToRegister(kind),
        {}, false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    addParam<IfNotEmpty>(_data, QStringLiteral("bind_email"), bindEmail);
    addParam<IfNotEmpty>(_data, QStringLiteral("username"), username);
    addParam<IfNotEmpty>(_data, QStringLiteral("password"), password);
    addParam<IfNotEmpty>(_data, QStringLiteral("device_id"), deviceId);
    addParam<IfNotEmpty>(_data, QStringLiteral("initial_device_display_name"), initialDeviceDisplayName);
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
    d->userId = fromJson<QString>(json.value("user_id"_ls));
    d->accessToken = fromJson<QString>(json.value("access_token"_ls));
    d->homeServer = fromJson<QString>(json.value("home_server"_ls));
    d->deviceId = fromJson<QString>(json.value("device_id"_ls));
    return Success;
}

static const auto RequestTokenToRegisterJobName = QStringLiteral("RequestTokenToRegisterJob");

RequestTokenToRegisterJob::RequestTokenToRegisterJob(const QString& clientSecret, const QString& email, int sendAttempt, const QString& idServer)
    : BaseJob(HttpVerb::Post, RequestTokenToRegisterJobName,
        basePath % "/register/email/requestToken", false)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("id_server"), idServer);
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("email"), email);
    addParam<>(_data, QStringLiteral("send_attempt"), sendAttempt);
    setRequestData(_data);
}

static const auto ChangePasswordJobName = QStringLiteral("ChangePasswordJob");

ChangePasswordJob::ChangePasswordJob(const QString& newPassword, const QJsonObject& auth)
    : BaseJob(HttpVerb::Post, ChangePasswordJobName,
        basePath % "/account/password")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("new_password"), newPassword);
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(_data);
}

QUrl RequestTokenToResetPasswordJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/account/password/email/requestToken");
}

static const auto RequestTokenToResetPasswordJobName = QStringLiteral("RequestTokenToResetPasswordJob");

RequestTokenToResetPasswordJob::RequestTokenToResetPasswordJob()
    : BaseJob(HttpVerb::Post, RequestTokenToResetPasswordJobName,
        basePath % "/account/password/email/requestToken", false)
{
}

static const auto DeactivateAccountJobName = QStringLiteral("DeactivateAccountJob");

DeactivateAccountJob::DeactivateAccountJob(const QJsonObject& auth)
    : BaseJob(HttpVerb::Post, DeactivateAccountJobName,
        basePath % "/account/deactivate")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
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
    addParam<>(_q, QStringLiteral("username"), username);
    return _q;
}

QUrl CheckUsernameAvailabilityJob::makeRequestUrl(QUrl baseUrl, const QString& username)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/register/available",
            queryToCheckUsernameAvailability(username));
}

static const auto CheckUsernameAvailabilityJobName = QStringLiteral("CheckUsernameAvailabilityJob");

CheckUsernameAvailabilityJob::CheckUsernameAvailabilityJob(const QString& username)
    : BaseJob(HttpVerb::Get, CheckUsernameAvailabilityJobName,
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
    d->available = fromJson<bool>(json.value("available"_ls));
    return Success;
}


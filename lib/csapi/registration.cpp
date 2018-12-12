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

RegisterJob::RegisterJob(const QString& kind, const Omittable<AuthenticationData>& auth, Omittable<bool> bindEmail, const QString& username, const QString& password, const QString& deviceId, const QString& initialDeviceDisplayName, Omittable<bool> inhibitLogin)
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
    addParam<IfNotEmpty>(_data, QStringLiteral("inhibit_login"), inhibitLogin);
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
    if (!json.contains("user_id"_ls))
        return { JsonParseError,
            "The key 'user_id' not found in the response" };
    d->userId = fromJson<QString>(json.value("user_id"_ls));
    d->accessToken = fromJson<QString>(json.value("access_token"_ls));
    d->homeServer = fromJson<QString>(json.value("home_server"_ls));
    d->deviceId = fromJson<QString>(json.value("device_id"_ls));
    return Success;
}

class RequestTokenToRegisterEmailJob::Private
{
    public:
        Sid data;
};

static const auto RequestTokenToRegisterEmailJobName = QStringLiteral("RequestTokenToRegisterEmailJob");

RequestTokenToRegisterEmailJob::RequestTokenToRegisterEmailJob(const QString& clientSecret, const QString& email, int sendAttempt, const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post, RequestTokenToRegisterEmailJobName,
        basePath % "/register/email/requestToken", false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("email"), email);
    addParam<>(_data, QStringLiteral("send_attempt"), sendAttempt);
    addParam<IfNotEmpty>(_data, QStringLiteral("next_link"), nextLink);
    addParam<>(_data, QStringLiteral("id_server"), idServer);
    setRequestData(_data);
}

RequestTokenToRegisterEmailJob::~RequestTokenToRegisterEmailJob() = default;

const Sid& RequestTokenToRegisterEmailJob::data() const
{
    return d->data;
}

BaseJob::Status RequestTokenToRegisterEmailJob::parseJson(const QJsonDocument& data)
{
    d->data = fromJson<Sid>(data);
    return Success;
}

class RequestTokenToRegisterMSISDNJob::Private
{
    public:
        Sid data;
};

static const auto RequestTokenToRegisterMSISDNJobName = QStringLiteral("RequestTokenToRegisterMSISDNJob");

RequestTokenToRegisterMSISDNJob::RequestTokenToRegisterMSISDNJob(const QString& clientSecret, const QString& country, const QString& phoneNumber, int sendAttempt, const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post, RequestTokenToRegisterMSISDNJobName,
        basePath % "/register/msisdn/requestToken", false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("country"), country);
    addParam<>(_data, QStringLiteral("phone_number"), phoneNumber);
    addParam<>(_data, QStringLiteral("send_attempt"), sendAttempt);
    addParam<IfNotEmpty>(_data, QStringLiteral("next_link"), nextLink);
    addParam<>(_data, QStringLiteral("id_server"), idServer);
    setRequestData(_data);
}

RequestTokenToRegisterMSISDNJob::~RequestTokenToRegisterMSISDNJob() = default;

const Sid& RequestTokenToRegisterMSISDNJob::data() const
{
    return d->data;
}

BaseJob::Status RequestTokenToRegisterMSISDNJob::parseJson(const QJsonDocument& data)
{
    d->data = fromJson<Sid>(data);
    return Success;
}

static const auto ChangePasswordJobName = QStringLiteral("ChangePasswordJob");

ChangePasswordJob::ChangePasswordJob(const QString& newPassword, const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, ChangePasswordJobName,
        basePath % "/account/password")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("new_password"), newPassword);
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(_data);
}

class RequestTokenToResetPasswordEmailJob::Private
{
    public:
        Sid data;
};

static const auto RequestTokenToResetPasswordEmailJobName = QStringLiteral("RequestTokenToResetPasswordEmailJob");

RequestTokenToResetPasswordEmailJob::RequestTokenToResetPasswordEmailJob(const QString& clientSecret, const QString& email, int sendAttempt, const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post, RequestTokenToResetPasswordEmailJobName,
        basePath % "/account/password/email/requestToken", false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("email"), email);
    addParam<>(_data, QStringLiteral("send_attempt"), sendAttempt);
    addParam<IfNotEmpty>(_data, QStringLiteral("next_link"), nextLink);
    addParam<>(_data, QStringLiteral("id_server"), idServer);
    setRequestData(_data);
}

RequestTokenToResetPasswordEmailJob::~RequestTokenToResetPasswordEmailJob() = default;

const Sid& RequestTokenToResetPasswordEmailJob::data() const
{
    return d->data;
}

BaseJob::Status RequestTokenToResetPasswordEmailJob::parseJson(const QJsonDocument& data)
{
    d->data = fromJson<Sid>(data);
    return Success;
}

class RequestTokenToResetPasswordMSISDNJob::Private
{
    public:
        Sid data;
};

static const auto RequestTokenToResetPasswordMSISDNJobName = QStringLiteral("RequestTokenToResetPasswordMSISDNJob");

RequestTokenToResetPasswordMSISDNJob::RequestTokenToResetPasswordMSISDNJob(const QString& clientSecret, const QString& country, const QString& phoneNumber, int sendAttempt, const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post, RequestTokenToResetPasswordMSISDNJobName,
        basePath % "/account/password/msisdn/requestToken", false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("country"), country);
    addParam<>(_data, QStringLiteral("phone_number"), phoneNumber);
    addParam<>(_data, QStringLiteral("send_attempt"), sendAttempt);
    addParam<IfNotEmpty>(_data, QStringLiteral("next_link"), nextLink);
    addParam<>(_data, QStringLiteral("id_server"), idServer);
    setRequestData(_data);
}

RequestTokenToResetPasswordMSISDNJob::~RequestTokenToResetPasswordMSISDNJob() = default;

const Sid& RequestTokenToResetPasswordMSISDNJob::data() const
{
    return d->data;
}

BaseJob::Status RequestTokenToResetPasswordMSISDNJob::parseJson(const QJsonDocument& data)
{
    d->data = fromJson<Sid>(data);
    return Success;
}

static const auto DeactivateAccountJobName = QStringLiteral("DeactivateAccountJob");

DeactivateAccountJob::DeactivateAccountJob(const Omittable<AuthenticationData>& auth)
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
        Omittable<bool> available;
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

Omittable<bool> CheckUsernameAvailabilityJob::available() const
{
    return d->available;
}

BaseJob::Status CheckUsernameAvailabilityJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->available = fromJson<bool>(json.value("available"_ls));
    return Success;
}


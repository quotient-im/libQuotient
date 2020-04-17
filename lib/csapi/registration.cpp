/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "registration.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class RegisterJob::Private {
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

RegisterJob::RegisterJob(const QString& kind,
                         const Omittable<AuthenticationData>& auth,
                         Omittable<bool> bindEmail, const QString& username,
                         const QString& password, const QString& deviceId,
                         const QString& initialDeviceDisplayName,
                         Omittable<bool> inhibitLogin)
    : BaseJob(HttpVerb::Post, QStringLiteral("RegisterJob"),
              basePath % "/register", queryToRegister(kind), {}, false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    addParam<IfNotEmpty>(_data, QStringLiteral("bind_email"), bindEmail);
    addParam<IfNotEmpty>(_data, QStringLiteral("username"), username);
    addParam<IfNotEmpty>(_data, QStringLiteral("password"), password);
    addParam<IfNotEmpty>(_data, QStringLiteral("device_id"), deviceId);
    addParam<IfNotEmpty>(_data, QStringLiteral("initial_device_display_name"),
                         initialDeviceDisplayName);
    addParam<IfNotEmpty>(_data, QStringLiteral("inhibit_login"), inhibitLogin);
    setRequestData(_data);
}

RegisterJob::~RegisterJob() = default;

const QString& RegisterJob::userId() const { return d->userId; }

const QString& RegisterJob::accessToken() const { return d->accessToken; }

const QString& RegisterJob::homeServer() const { return d->homeServer; }

const QString& RegisterJob::deviceId() const { return d->deviceId; }

BaseJob::Status RegisterJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("user_id"_ls))
        return { IncorrectResponse,
                 "The key 'user_id' not found in the response" };
    fromJson(json.value("user_id"_ls), d->userId);
    fromJson(json.value("access_token"_ls), d->accessToken);
    fromJson(json.value("home_server"_ls), d->homeServer);
    fromJson(json.value("device_id"_ls), d->deviceId);

    return Success;
}

class RequestTokenToRegisterEmailJob::Private {
public:
    Sid data;
};

RequestTokenToRegisterEmailJob::RequestTokenToRegisterEmailJob(
    const QString& clientSecret, const QString& email, int sendAttempt,
    const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToRegisterEmailJob"),
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

const Sid& RequestTokenToRegisterEmailJob::data() const { return d->data; }

BaseJob::Status
RequestTokenToRegisterEmailJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

class RequestTokenToRegisterMSISDNJob::Private {
public:
    Sid data;
};

RequestTokenToRegisterMSISDNJob::RequestTokenToRegisterMSISDNJob(
    const QString& clientSecret, const QString& country,
    const QString& phoneNumber, int sendAttempt, const QString& idServer,
    const QString& nextLink)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToRegisterMSISDNJob"),
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

const Sid& RequestTokenToRegisterMSISDNJob::data() const { return d->data; }

BaseJob::Status
RequestTokenToRegisterMSISDNJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

ChangePasswordJob::ChangePasswordJob(const QString& newPassword,
                                     const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("ChangePasswordJob"),
              basePath % "/account/password")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("new_password"), newPassword);
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(_data);
}

class RequestTokenToResetPasswordEmailJob::Private {
public:
    Sid data;
};

RequestTokenToResetPasswordEmailJob::RequestTokenToResetPasswordEmailJob(
    const QString& clientSecret, const QString& email, int sendAttempt,
    const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post,
              QStringLiteral("RequestTokenToResetPasswordEmailJob"),
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

RequestTokenToResetPasswordEmailJob::~RequestTokenToResetPasswordEmailJob() =
    default;

const Sid& RequestTokenToResetPasswordEmailJob::data() const { return d->data; }

BaseJob::Status
RequestTokenToResetPasswordEmailJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

class RequestTokenToResetPasswordMSISDNJob::Private {
public:
    Sid data;
};

RequestTokenToResetPasswordMSISDNJob::RequestTokenToResetPasswordMSISDNJob(
    const QString& clientSecret, const QString& country,
    const QString& phoneNumber, int sendAttempt, const QString& idServer,
    const QString& nextLink)
    : BaseJob(HttpVerb::Post,
              QStringLiteral("RequestTokenToResetPasswordMSISDNJob"),
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

RequestTokenToResetPasswordMSISDNJob::~RequestTokenToResetPasswordMSISDNJob() =
    default;

const Sid& RequestTokenToResetPasswordMSISDNJob::data() const
{
    return d->data;
}

BaseJob::Status
RequestTokenToResetPasswordMSISDNJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

DeactivateAccountJob::DeactivateAccountJob(
    const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("DeactivateAccountJob"),
              basePath % "/account/deactivate")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(_data);
}

class CheckUsernameAvailabilityJob::Private {
public:
    Omittable<bool> available;
};

BaseJob::Query queryToCheckUsernameAvailability(const QString& username)
{
    BaseJob::Query _q;
    addParam<>(_q, QStringLiteral("username"), username);
    return _q;
}

QUrl CheckUsernameAvailabilityJob::makeRequestUrl(QUrl baseUrl,
                                                  const QString& username)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/register/available",
                                   queryToCheckUsernameAvailability(username));
}

CheckUsernameAvailabilityJob::CheckUsernameAvailabilityJob(const QString& username)
    : BaseJob(HttpVerb::Get, QStringLiteral("CheckUsernameAvailabilityJob"),
              basePath % "/register/available",
              queryToCheckUsernameAvailability(username), {}, false)
    , d(new Private)
{}

CheckUsernameAvailabilityJob::~CheckUsernameAvailabilityJob() = default;

Omittable<bool> CheckUsernameAvailabilityJob::available() const
{
    return d->available;
}

BaseJob::Status CheckUsernameAvailabilityJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("available"_ls), d->available);

    return Success;
}

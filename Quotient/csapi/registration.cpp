// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "registration.h"

using namespace Quotient;

auto queryToRegister(const QString& kind)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("kind"), kind);
    return _q;
}

RegisterJob::RegisterJob(const QString& kind, const Omittable<AuthenticationData>& auth,
                         const QString& username, const QString& password, const QString& deviceId,
                         const QString& initialDeviceDisplayName, Omittable<bool> inhibitLogin,
                         Omittable<bool> refreshToken)
    : BaseJob(HttpVerb::Post, QStringLiteral("RegisterJob"),
              makePath("/_matrix/client/v3", "/register"), queryToRegister(kind), {}, false)
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("auth"), auth);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("username"), username);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("password"), password);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("device_id"), deviceId);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("initial_device_display_name"),
                         initialDeviceDisplayName);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("inhibit_login"), inhibitLogin);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("refresh_token"), refreshToken);
    setRequestData({ _dataJson });
    addExpectedKey("user_id");
}

RequestTokenToRegisterEmailJob::RequestTokenToRegisterEmailJob(const EmailValidationData& data)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToRegisterEmailJob"),
              makePath("/_matrix/client/v3", "/register/email/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

RequestTokenToRegisterMSISDNJob::RequestTokenToRegisterMSISDNJob(const MsisdnValidationData& data)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToRegisterMSISDNJob"),
              makePath("/_matrix/client/v3", "/register/msisdn/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

ChangePasswordJob::ChangePasswordJob(const QString& newPassword, bool logoutDevices,
                                     const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("ChangePasswordJob"),
              makePath("/_matrix/client/v3", "/account/password"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("new_password"), newPassword);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("logout_devices"), logoutDevices);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("auth"), auth);
    setRequestData({ _dataJson });
}

RequestTokenToResetPasswordEmailJob::RequestTokenToResetPasswordEmailJob(
    const EmailValidationData& data)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToResetPasswordEmailJob"),
              makePath("/_matrix/client/v3", "/account/password/email/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

RequestTokenToResetPasswordMSISDNJob::RequestTokenToResetPasswordMSISDNJob(
    const MsisdnValidationData& data)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToResetPasswordMSISDNJob"),
              makePath("/_matrix/client/v3", "/account/password/msisdn/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

DeactivateAccountJob::DeactivateAccountJob(const Omittable<AuthenticationData>& auth,
                                           const QString& idServer)
    : BaseJob(HttpVerb::Post, QStringLiteral("DeactivateAccountJob"),
              makePath("/_matrix/client/v3", "/account/deactivate"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("auth"), auth);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("id_server"), idServer);
    setRequestData({ _dataJson });
    addExpectedKey("id_server_unbind_result");
}

auto queryToCheckUsernameAvailability(const QString& username)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("username"), username);
    return _q;
}

QUrl CheckUsernameAvailabilityJob::makeRequestUrl(QUrl baseUrl, const QString& username)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/v3", "/register/available"),
                                   queryToCheckUsernameAvailability(username));
}

CheckUsernameAvailabilityJob::CheckUsernameAvailabilityJob(const QString& username)
    : BaseJob(HttpVerb::Get, QStringLiteral("CheckUsernameAvailabilityJob"),
              makePath("/_matrix/client/v3", "/register/available"),
              queryToCheckUsernameAvailability(username), {}, false)
{}

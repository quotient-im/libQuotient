// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "registration.h"

using namespace Quotient;

auto queryToRegister(const QString& kind)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, u"kind"_s, kind);
    return _q;
}

RegisterJob::RegisterJob(const QString& kind, const std::optional<AuthenticationData>& auth,
                         const QString& username, const QString& password, const QString& deviceId,
                         const QString& initialDeviceDisplayName, std::optional<bool> inhibitLogin,
                         std::optional<bool> refreshToken)
    : BaseJob(HttpVerb::Post, u"RegisterJob"_s, makePath("/_matrix/client/v3", "/register"),
              queryToRegister(kind), {}, false)
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "auth"_L1, auth);
    addParam<IfNotEmpty>(_dataJson, "username"_L1, username);
    addParam<IfNotEmpty>(_dataJson, "password"_L1, password);
    addParam<IfNotEmpty>(_dataJson, "device_id"_L1, deviceId);
    addParam<IfNotEmpty>(_dataJson, "initial_device_display_name"_L1, initialDeviceDisplayName);
    addParam<IfNotEmpty>(_dataJson, "inhibit_login"_L1, inhibitLogin);
    addParam<IfNotEmpty>(_dataJson, "refresh_token"_L1, refreshToken);
    setRequestData({ _dataJson });
    addExpectedKey("user_id");
}

RequestTokenToRegisterEmailJob::RequestTokenToRegisterEmailJob(const EmailValidationData& data)
    : BaseJob(HttpVerb::Post, u"RequestTokenToRegisterEmailJob"_s,
              makePath("/_matrix/client/v3", "/register/email/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

RequestTokenToRegisterMSISDNJob::RequestTokenToRegisterMSISDNJob(const MsisdnValidationData& data)
    : BaseJob(HttpVerb::Post, u"RequestTokenToRegisterMSISDNJob"_s,
              makePath("/_matrix/client/v3", "/register/msisdn/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

ChangePasswordJob::ChangePasswordJob(const QString& newPassword, bool logoutDevices,
                                     const std::optional<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, u"ChangePasswordJob"_s,
              makePath("/_matrix/client/v3", "/account/password"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "new_password"_L1, newPassword);
    addParam<IfNotEmpty>(_dataJson, "logout_devices"_L1, logoutDevices);
    addParam<IfNotEmpty>(_dataJson, "auth"_L1, auth);
    setRequestData({ _dataJson });
}

RequestTokenToResetPasswordEmailJob::RequestTokenToResetPasswordEmailJob(
    const EmailValidationData& data)
    : BaseJob(HttpVerb::Post, u"RequestTokenToResetPasswordEmailJob"_s,
              makePath("/_matrix/client/v3", "/account/password/email/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

RequestTokenToResetPasswordMSISDNJob::RequestTokenToResetPasswordMSISDNJob(
    const MsisdnValidationData& data)
    : BaseJob(HttpVerb::Post, u"RequestTokenToResetPasswordMSISDNJob"_s,
              makePath("/_matrix/client/v3", "/account/password/msisdn/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

DeactivateAccountJob::DeactivateAccountJob(const std::optional<AuthenticationData>& auth,
                                           const QString& idServer, std::optional<bool> erase)
    : BaseJob(HttpVerb::Post, u"DeactivateAccountJob"_s,
              makePath("/_matrix/client/v3", "/account/deactivate"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "auth"_L1, auth);
    addParam<IfNotEmpty>(_dataJson, "id_server"_L1, idServer);
    addParam<IfNotEmpty>(_dataJson, "erase"_L1, erase);
    setRequestData({ _dataJson });
    addExpectedKey("id_server_unbind_result");
}

auto queryToCheckUsernameAvailability(const QString& username)
{
    QUrlQuery _q;
    addParam<>(_q, u"username"_s, username);
    return _q;
}

QUrl CheckUsernameAvailabilityJob::makeRequestUrl(const HomeserverData& hsData,
                                                  const QString& username)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/register/available"),
                                   queryToCheckUsernameAvailability(username));
}

CheckUsernameAvailabilityJob::CheckUsernameAvailabilityJob(const QString& username)
    : BaseJob(HttpVerb::Get, u"CheckUsernameAvailabilityJob"_s,
              makePath("/_matrix/client/v3", "/register/available"),
              queryToCheckUsernameAvailability(username), {}, false)
{}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "registration.h"

using namespace Quotient;

auto queryToRegister(const QString& kind)
{
    QUrlQuery _q;
    addParam<IfNotEmpty>(_q, QStringLiteral("kind"), kind);
    return _q;
}

RegisterJob::RegisterJob(const QString& kind,
                         const Omittable<AuthenticationData>& auth,
                         const QString& username, const QString& password,
                         const QString& deviceId,
                         const QString& initialDeviceDisplayName,
                         Omittable<bool> inhibitLogin)
    : BaseJob(HttpVerb::Post, QStringLiteral("RegisterJob"),
              makePath("/_matrix/client/r0", "/register"),
              queryToRegister(kind), {}, false)
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    addParam<IfNotEmpty>(_data, QStringLiteral("username"), username);
    addParam<IfNotEmpty>(_data, QStringLiteral("password"), password);
    addParam<IfNotEmpty>(_data, QStringLiteral("device_id"), deviceId);
    addParam<IfNotEmpty>(_data, QStringLiteral("initial_device_display_name"),
                         initialDeviceDisplayName);
    addParam<IfNotEmpty>(_data, QStringLiteral("inhibit_login"), inhibitLogin);
    setRequestData(std::move(_data));
    addExpectedKey("user_id");
}

RequestTokenToRegisterEmailJob::RequestTokenToRegisterEmailJob(
    const EmailValidationData& body)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToRegisterEmailJob"),
              makePath("/_matrix/client/r0", "/register/email/requestToken"),
              false)
{
    setRequestData(RequestData(toJson(body)));
}

RequestTokenToRegisterMSISDNJob::RequestTokenToRegisterMSISDNJob(
    const MsisdnValidationData& body)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToRegisterMSISDNJob"),
              makePath("/_matrix/client/r0", "/register/msisdn/requestToken"),
              false)
{
    setRequestData(RequestData(toJson(body)));
}

ChangePasswordJob::ChangePasswordJob(const QString& newPassword,
                                     bool logoutDevices,
                                     const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("ChangePasswordJob"),
              makePath("/_matrix/client/r0", "/account/password"))
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("new_password"), newPassword);
    addParam<IfNotEmpty>(_data, QStringLiteral("logout_devices"), logoutDevices);
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    setRequestData(std::move(_data));
}

RequestTokenToResetPasswordEmailJob::RequestTokenToResetPasswordEmailJob(
    const EmailValidationData& body)
    : BaseJob(HttpVerb::Post,
              QStringLiteral("RequestTokenToResetPasswordEmailJob"),
              makePath("/_matrix/client/r0",
                       "/account/password/email/requestToken"),
              false)
{
    setRequestData(RequestData(toJson(body)));
}

RequestTokenToResetPasswordMSISDNJob::RequestTokenToResetPasswordMSISDNJob(
    const MsisdnValidationData& body)
    : BaseJob(HttpVerb::Post,
              QStringLiteral("RequestTokenToResetPasswordMSISDNJob"),
              makePath("/_matrix/client/r0",
                       "/account/password/msisdn/requestToken"),
              false)
{
    setRequestData(RequestData(toJson(body)));
}

DeactivateAccountJob::DeactivateAccountJob(
    const Omittable<AuthenticationData>& auth, const QString& idServer)
    : BaseJob(HttpVerb::Post, QStringLiteral("DeactivateAccountJob"),
              makePath("/_matrix/client/r0", "/account/deactivate"))
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    addParam<IfNotEmpty>(_data, QStringLiteral("id_server"), idServer);
    setRequestData(std::move(_data));
    addExpectedKey("id_server_unbind_result");
}

auto queryToCheckUsernameAvailability(const QString& username)
{
    QUrlQuery _q;
    addParam<>(_q, QStringLiteral("username"), username);
    return _q;
}

QUrl CheckUsernameAvailabilityJob::makeRequestUrl(QUrl baseUrl,
                                                  const QString& username)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/r0",
                                            "/register/available"),
                                   queryToCheckUsernameAvailability(username));
}

CheckUsernameAvailabilityJob::CheckUsernameAvailabilityJob(const QString& username)
    : BaseJob(HttpVerb::Get, QStringLiteral("CheckUsernameAvailabilityJob"),
              makePath("/_matrix/client/r0", "/register/available"),
              queryToCheckUsernameAvailability(username), {}, false)
{}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "registration.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

auto queryToRegister(const QString& kind)
{
    BaseJob::Query _q;
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
              QStringLiteral("/_matrix/client/r0") % "/register",
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
              QStringLiteral("/_matrix/client/r0")
                  % "/register/email/requestToken",
              false)
{
    setRequestData(Data(toJson(body)));
}

RequestTokenToRegisterMSISDNJob::RequestTokenToRegisterMSISDNJob(
    const MsisdnValidationData& body)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenToRegisterMSISDNJob"),
              QStringLiteral("/_matrix/client/r0")
                  % "/register/msisdn/requestToken",
              false)
{
    setRequestData(Data(toJson(body)));
}

ChangePasswordJob::ChangePasswordJob(const QString& newPassword,
                                     bool logoutDevices,
                                     const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("ChangePasswordJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/password")
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
              QStringLiteral("/_matrix/client/r0")
                  % "/account/password/email/requestToken",
              false)
{
    setRequestData(Data(toJson(body)));
}

RequestTokenToResetPasswordMSISDNJob::RequestTokenToResetPasswordMSISDNJob(
    const MsisdnValidationData& body)
    : BaseJob(HttpVerb::Post,
              QStringLiteral("RequestTokenToResetPasswordMSISDNJob"),
              QStringLiteral("/_matrix/client/r0")
                  % "/account/password/msisdn/requestToken",
              false)
{
    setRequestData(Data(toJson(body)));
}

DeactivateAccountJob::DeactivateAccountJob(
    const Omittable<AuthenticationData>& auth, const QString& idServer)
    : BaseJob(HttpVerb::Post, QStringLiteral("DeactivateAccountJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/deactivate")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    addParam<IfNotEmpty>(_data, QStringLiteral("id_server"), idServer);
    setRequestData(std::move(_data));
    addExpectedKey("id_server_unbind_result");
}

auto queryToCheckUsernameAvailability(const QString& username)
{
    BaseJob::Query _q;
    addParam<>(_q, QStringLiteral("username"), username);
    return _q;
}

QUrl CheckUsernameAvailabilityJob::makeRequestUrl(QUrl baseUrl,
                                                  const QString& username)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/register/available",
                                   queryToCheckUsernameAvailability(username));
}

CheckUsernameAvailabilityJob::CheckUsernameAvailabilityJob(const QString& username)
    : BaseJob(HttpVerb::Get, QStringLiteral("CheckUsernameAvailabilityJob"),
              QStringLiteral("/_matrix/client/r0") % "/register/available",
              queryToCheckUsernameAvailability(username), {}, false)
{}

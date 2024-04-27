// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "login.h"

using namespace Quotient;

QUrl GetLoginFlowsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), makePath("/_matrix/client/v3", "/login"));
}

GetLoginFlowsJob::GetLoginFlowsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetLoginFlowsJob"),
              makePath("/_matrix/client/v3", "/login"), false)
{}

LoginJob::LoginJob(const QString& type, const std::optional<UserIdentifier>& identifier,
                   const QString& password, const QString& token, const QString& deviceId,
                   const QString& initialDeviceDisplayName, std::optional<bool> refreshToken)
    : BaseJob(HttpVerb::Post, QStringLiteral("LoginJob"), makePath("/_matrix/client/v3", "/login"),
              false)
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, QStringLiteral("type"), type);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("identifier"), identifier);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("password"), password);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("token"), token);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("device_id"), deviceId);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("initial_device_display_name"),
                         initialDeviceDisplayName);
    addParam<IfNotEmpty>(_dataJson, QStringLiteral("refresh_token"), refreshToken);
    setRequestData({ _dataJson });
    addExpectedKey("user_id");
    addExpectedKey("access_token");
    addExpectedKey("device_id");
}

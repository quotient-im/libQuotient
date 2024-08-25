// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "login.h"

using namespace Quotient;

QUrl GetLoginFlowsJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/login"));
}

GetLoginFlowsJob::GetLoginFlowsJob()
    : BaseJob(HttpVerb::Get, u"GetLoginFlowsJob"_s, makePath("/_matrix/client/v3", "/login"), false)
{}

LoginJob::LoginJob(const QString& type, const std::optional<UserIdentifier>& identifier,
                   const QString& password, const QString& token, const QString& deviceId,
                   const QString& initialDeviceDisplayName, std::optional<bool> refreshToken)
    : BaseJob(HttpVerb::Post, u"LoginJob"_s, makePath("/_matrix/client/v3", "/login"), false)
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "type"_L1, type);
    addParam<IfNotEmpty>(_dataJson, "identifier"_L1, identifier);
    addParam<IfNotEmpty>(_dataJson, "password"_L1, password);
    addParam<IfNotEmpty>(_dataJson, "token"_L1, token);
    addParam<IfNotEmpty>(_dataJson, "device_id"_L1, deviceId);
    addParam<IfNotEmpty>(_dataJson, "initial_device_display_name"_L1, initialDeviceDisplayName);
    addParam<IfNotEmpty>(_dataJson, "refresh_token"_L1, refreshToken);
    setRequestData({ _dataJson });
    addExpectedKey("user_id");
    addExpectedKey("access_token");
    addExpectedKey("device_id");
}

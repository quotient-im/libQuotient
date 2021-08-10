/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "login.h"

using namespace Quotient;

QUrl GetLoginFlowsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   makePath("/_matrix/client/r0", "/login"));
}

GetLoginFlowsJob::GetLoginFlowsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetLoginFlowsJob"),
              makePath("/_matrix/client/r0", "/login"), false)
{}

LoginJob::LoginJob(const QString& type,
                   const Omittable<UserIdentifier>& identifier,
                   const QString& password, const QString& token,
                   const QString& deviceId,
                   const QString& initialDeviceDisplayName)
    : BaseJob(HttpVerb::Post, QStringLiteral("LoginJob"),
              makePath("/_matrix/client/r0", "/login"), false)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("type"), type);
    addParam<IfNotEmpty>(_data, QStringLiteral("identifier"), identifier);
    addParam<IfNotEmpty>(_data, QStringLiteral("password"), password);
    addParam<IfNotEmpty>(_data, QStringLiteral("token"), token);
    addParam<IfNotEmpty>(_data, QStringLiteral("device_id"), deviceId);
    addParam<IfNotEmpty>(_data, QStringLiteral("initial_device_display_name"),
                         initialDeviceDisplayName);
    setRequestData(std::move(_data));
}

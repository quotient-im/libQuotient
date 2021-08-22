/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "administrative_contact.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

QUrl GetAccount3PIDsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   QStringLiteral("/_matrix/client/r0")
                                       % "/account/3pid");
}

GetAccount3PIDsJob::GetAccount3PIDsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetAccount3PIDsJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/3pid")
{}

Post3PIDsJob::Post3PIDsJob(const ThreePidCredentials& threePidCreds)
    : BaseJob(HttpVerb::Post, QStringLiteral("Post3PIDsJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/3pid")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("three_pid_creds"), threePidCreds);
    setRequestData(std::move(_data));
}

Add3PIDJob::Add3PIDJob(const QString& clientSecret, const QString& sid,
                       const Omittable<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, QStringLiteral("Add3PIDJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/3pid/add")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("auth"), auth);
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("sid"), sid);
    setRequestData(std::move(_data));
}

Bind3PIDJob::Bind3PIDJob(const QString& clientSecret, const QString& idServer,
                         const QString& idAccessToken, const QString& sid)
    : BaseJob(HttpVerb::Post, QStringLiteral("Bind3PIDJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/3pid/bind")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("id_server"), idServer);
    addParam<>(_data, QStringLiteral("id_access_token"), idAccessToken);
    addParam<>(_data, QStringLiteral("sid"), sid);
    setRequestData(std::move(_data));
}

Delete3pidFromAccountJob::Delete3pidFromAccountJob(const QString& medium,
                                                   const QString& address,
                                                   const QString& idServer)
    : BaseJob(HttpVerb::Post, QStringLiteral("Delete3pidFromAccountJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/3pid/delete")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("id_server"), idServer);
    addParam<>(_data, QStringLiteral("medium"), medium);
    addParam<>(_data, QStringLiteral("address"), address);
    setRequestData(std::move(_data));
    addExpectedKey("id_server_unbind_result");
}

Unbind3pidFromAccountJob::Unbind3pidFromAccountJob(const QString& medium,
                                                   const QString& address,
                                                   const QString& idServer)
    : BaseJob(HttpVerb::Post, QStringLiteral("Unbind3pidFromAccountJob"),
              QStringLiteral("/_matrix/client/r0") % "/account/3pid/unbind")
{
    QJsonObject _data;
    addParam<IfNotEmpty>(_data, QStringLiteral("id_server"), idServer);
    addParam<>(_data, QStringLiteral("medium"), medium);
    addParam<>(_data, QStringLiteral("address"), address);
    setRequestData(std::move(_data));
    addExpectedKey("id_server_unbind_result");
}

RequestTokenTo3PIDEmailJob::RequestTokenTo3PIDEmailJob(
    const EmailValidationData& body)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenTo3PIDEmailJob"),
              QStringLiteral("/_matrix/client/r0")
                  % "/account/3pid/email/requestToken",
              false)
{
    setRequestData(RequestData(toJson(body)));
}

RequestTokenTo3PIDMSISDNJob::RequestTokenTo3PIDMSISDNJob(
    const MsisdnValidationData& body)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenTo3PIDMSISDNJob"),
              QStringLiteral("/_matrix/client/r0")
                  % "/account/3pid/msisdn/requestToken",
              false)
{
    setRequestData(RequestData(toJson(body)));
}

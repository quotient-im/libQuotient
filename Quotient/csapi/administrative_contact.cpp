// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

#include "administrative_contact.h"

using namespace Quotient;

QUrl GetAccount3PIDsJob::makeRequestUrl(const HomeserverData& hsData)
{
    return BaseJob::makeRequestUrl(hsData, makePath("/_matrix/client/v3", "/account/3pid"));
}

GetAccount3PIDsJob::GetAccount3PIDsJob()
    : BaseJob(HttpVerb::Get, u"GetAccount3PIDsJob"_s,
              makePath("/_matrix/client/v3", "/account/3pid"))
{}

Post3PIDsJob::Post3PIDsJob(const ThreePidCredentials& threePidCreds)
    : BaseJob(HttpVerb::Post, u"Post3PIDsJob"_s, makePath("/_matrix/client/v3", "/account/3pid"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "three_pid_creds"_L1, threePidCreds);
    setRequestData({ _dataJson });
}

Add3PIDJob::Add3PIDJob(const QString& clientSecret, const QString& sid,
                       const std::optional<AuthenticationData>& auth)
    : BaseJob(HttpVerb::Post, u"Add3PIDJob"_s, makePath("/_matrix/client/v3", "/account/3pid/add"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "auth"_L1, auth);
    addParam<>(_dataJson, "client_secret"_L1, clientSecret);
    addParam<>(_dataJson, "sid"_L1, sid);
    setRequestData({ _dataJson });
}

Bind3PIDJob::Bind3PIDJob(const QString& clientSecret, const QString& idServer,
                         const QString& idAccessToken, const QString& sid)
    : BaseJob(HttpVerb::Post, u"Bind3PIDJob"_s, makePath("/_matrix/client/v3", "/account/3pid/bind"))
{
    QJsonObject _dataJson;
    addParam<>(_dataJson, "client_secret"_L1, clientSecret);
    addParam<>(_dataJson, "id_server"_L1, idServer);
    addParam<>(_dataJson, "id_access_token"_L1, idAccessToken);
    addParam<>(_dataJson, "sid"_L1, sid);
    setRequestData({ _dataJson });
}

Delete3pidFromAccountJob::Delete3pidFromAccountJob(const QString& medium, const QString& address,
                                                   const QString& idServer)
    : BaseJob(HttpVerb::Post, u"Delete3pidFromAccountJob"_s,
              makePath("/_matrix/client/v3", "/account/3pid/delete"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "id_server"_L1, idServer);
    addParam<>(_dataJson, "medium"_L1, medium);
    addParam<>(_dataJson, "address"_L1, address);
    setRequestData({ _dataJson });
    addExpectedKey("id_server_unbind_result");
}

Unbind3pidFromAccountJob::Unbind3pidFromAccountJob(const QString& medium, const QString& address,
                                                   const QString& idServer)
    : BaseJob(HttpVerb::Post, u"Unbind3pidFromAccountJob"_s,
              makePath("/_matrix/client/v3", "/account/3pid/unbind"))
{
    QJsonObject _dataJson;
    addParam<IfNotEmpty>(_dataJson, "id_server"_L1, idServer);
    addParam<>(_dataJson, "medium"_L1, medium);
    addParam<>(_dataJson, "address"_L1, address);
    setRequestData({ _dataJson });
    addExpectedKey("id_server_unbind_result");
}

RequestTokenTo3PIDEmailJob::RequestTokenTo3PIDEmailJob(const EmailValidationData& data)
    : BaseJob(HttpVerb::Post, u"RequestTokenTo3PIDEmailJob"_s,
              makePath("/_matrix/client/v3", "/account/3pid/email/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

RequestTokenTo3PIDMSISDNJob::RequestTokenTo3PIDMSISDNJob(const MsisdnValidationData& data)
    : BaseJob(HttpVerb::Post, u"RequestTokenTo3PIDMSISDNJob"_s,
              makePath("/_matrix/client/v3", "/account/3pid/msisdn/requestToken"), false)
{
    setRequestData({ toJson(data) });
}

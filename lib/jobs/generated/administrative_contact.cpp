/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "administrative_contact.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

GetAccount3PIDsJob::ThirdPartyIdentifier::operator QJsonObject() const
{
    QJsonObject o;
    o.insert("medium", toJson(medium));
    o.insert("address", toJson(address));
    
    return o;
}
namespace QMatrixClient
{
    template <> struct FromJson<GetAccount3PIDsJob::ThirdPartyIdentifier>
    {
        GetAccount3PIDsJob::ThirdPartyIdentifier operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            GetAccount3PIDsJob::ThirdPartyIdentifier result;
            result.medium =
                fromJson<QString>(o.value("medium"));
            result.address =
                fromJson<QString>(o.value("address"));
            
            return result;
        }
    };
} // namespace QMatrixClient

class GetAccount3PIDsJob::Private
{
    public:
        QVector<ThirdPartyIdentifier> threepids;
};

QUrl GetAccount3PIDsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(baseUrl,
            basePath % "/account/3pid");
}

GetAccount3PIDsJob::GetAccount3PIDsJob()
    : BaseJob(HttpVerb::Get, "GetAccount3PIDsJob",
        basePath % "/account/3pid")
    , d(new Private)
{
}

GetAccount3PIDsJob::~GetAccount3PIDsJob() = default;

const QVector<GetAccount3PIDsJob::ThirdPartyIdentifier>& GetAccount3PIDsJob::threepids() const
{
    return d->threepids;
}

BaseJob::Status GetAccount3PIDsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    d->threepids = fromJson<QVector<ThirdPartyIdentifier>>(json.value("threepids"));
    return Success;
}

Post3PIDsJob::ThreePidCredentials::operator QJsonObject() const
{
    QJsonObject o;
    o.insert("client_secret", toJson(clientSecret));
    o.insert("id_server", toJson(idServer));
    o.insert("sid", toJson(sid));
    
    return o;
}
namespace QMatrixClient
{
    template <> struct FromJson<Post3PIDsJob::ThreePidCredentials>
    {
        Post3PIDsJob::ThreePidCredentials operator()(const QJsonValue& jv)
        {
            const auto& o = jv.toObject();
            Post3PIDsJob::ThreePidCredentials result;
            result.clientSecret =
                fromJson<QString>(o.value("client_secret"));
            result.idServer =
                fromJson<QString>(o.value("id_server"));
            result.sid =
                fromJson<QString>(o.value("sid"));
            
            return result;
        }
    };
} // namespace QMatrixClient

Post3PIDsJob::Post3PIDsJob(const ThreePidCredentials& threePidCreds, bool bind)
    : BaseJob(HttpVerb::Post, "Post3PIDsJob",
        basePath % "/account/3pid")
{
    QJsonObject _data;
    _data.insert("three_pid_creds", toJson(threePidCreds));
    _data.insert("bind", toJson(bind));
    setRequestData(_data);
}

QUrl RequestTokenTo3PIDJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(baseUrl,
            basePath % "/account/3pid/email/requestToken");
}

RequestTokenTo3PIDJob::RequestTokenTo3PIDJob()
    : BaseJob(HttpVerb::Post, "RequestTokenTo3PIDJob",
        basePath % "/account/3pid/email/requestToken", false)
{
}


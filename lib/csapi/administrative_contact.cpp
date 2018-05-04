/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "administrative_contact.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

namespace QMatrixClient
{
    // Converters

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
    return BaseJob::makeRequestUrl(std::move(baseUrl),
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

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const Post3PIDsJob::ThreePidCredentials& pod)
    {
        QJsonObject o;
        o.insert("client_secret", toJson(pod.clientSecret));
        o.insert("id_server", toJson(pod.idServer));
        o.insert("sid", toJson(pod.sid));

        return o;
    }
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
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/account/3pid/email/requestToken");
}

RequestTokenTo3PIDJob::RequestTokenTo3PIDJob()
    : BaseJob(HttpVerb::Post, "RequestTokenTo3PIDJob",
        basePath % "/account/3pid/email/requestToken", false)
{
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "administrative_contact.h"

#include "converters.h"

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
            const auto& _json = jv.toObject();
            GetAccount3PIDsJob::ThirdPartyIdentifier result;
            result.medium =
                fromJson<QString>(_json.value("medium"));
            result.address =
                fromJson<QString>(_json.value("address"));

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
        QJsonObject _json;
        addToJson<>(_json, "client_secret", pod.clientSecret);
        addToJson<>(_json, "id_server", pod.idServer);
        addToJson<>(_json, "sid", pod.sid);
        return _json;
    }
} // namespace QMatrixClient

Post3PIDsJob::Post3PIDsJob(const ThreePidCredentials& threePidCreds, bool bind)
    : BaseJob(HttpVerb::Post, "Post3PIDsJob",
        basePath % "/account/3pid")
{
    QJsonObject _data;
    addToJson<>(_data, "three_pid_creds", threePidCreds);
    addToJson<IfNotEmpty>(_data, "bind", bind);
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


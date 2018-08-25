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
                fromJson<QString>(_json.value("medium"_ls));
            result.address =
                fromJson<QString>(_json.value("address"_ls));

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

static const auto GetAccount3PIDsJobName = QStringLiteral("GetAccount3PIDsJob");

GetAccount3PIDsJob::GetAccount3PIDsJob()
    : BaseJob(HttpVerb::Get, GetAccount3PIDsJobName,
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
    d->threepids = fromJson<QVector<ThirdPartyIdentifier>>(json.value("threepids"_ls));
    return Success;
}

namespace QMatrixClient
{
    // Converters

    QJsonObject toJson(const Post3PIDsJob::ThreePidCredentials& pod)
    {
        QJsonObject _json;
        addParam<>(_json, QStringLiteral("client_secret"), pod.clientSecret);
        addParam<>(_json, QStringLiteral("id_server"), pod.idServer);
        addParam<>(_json, QStringLiteral("sid"), pod.sid);
        return _json;
    }
} // namespace QMatrixClient

static const auto Post3PIDsJobName = QStringLiteral("Post3PIDsJob");

Post3PIDsJob::Post3PIDsJob(const ThreePidCredentials& threePidCreds, bool bind)
    : BaseJob(HttpVerb::Post, Post3PIDsJobName,
        basePath % "/account/3pid")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("three_pid_creds"), threePidCreds);
    addParam<IfNotEmpty>(_data, QStringLiteral("bind"), bind);
    setRequestData(_data);
}

QUrl RequestTokenTo3PIDEmailJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/account/3pid/email/requestToken");
}

static const auto RequestTokenTo3PIDEmailJobName = QStringLiteral("RequestTokenTo3PIDEmailJob");

RequestTokenTo3PIDEmailJob::RequestTokenTo3PIDEmailJob()
    : BaseJob(HttpVerb::Post, RequestTokenTo3PIDEmailJobName,
        basePath % "/account/3pid/email/requestToken", false)
{
}

QUrl RequestTokenTo3PIDMSISDNJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
            basePath % "/account/3pid/msisdn/requestToken");
}

static const auto RequestTokenTo3PIDMSISDNJobName = QStringLiteral("RequestTokenTo3PIDMSISDNJob");

RequestTokenTo3PIDMSISDNJob::RequestTokenTo3PIDMSISDNJob()
    : BaseJob(HttpVerb::Post, RequestTokenTo3PIDMSISDNJobName,
        basePath % "/account/3pid/msisdn/requestToken", false)
{
}


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

    template <> struct FromJsonObject<GetAccount3PIDsJob::ThirdPartyIdentifier>
    {
        GetAccount3PIDsJob::ThirdPartyIdentifier operator()(const QJsonObject& jo) const
        {
            GetAccount3PIDsJob::ThirdPartyIdentifier result;
            result.medium =
                fromJson<QString>(jo.value("medium"_ls));
            result.address =
                fromJson<QString>(jo.value("address"_ls));
            result.validatedAt =
                fromJson<qint64>(jo.value("validated_at"_ls));
            result.addedAt =
                fromJson<qint64>(jo.value("added_at"_ls));

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
        QJsonObject jo;
        addParam<>(jo, QStringLiteral("client_secret"), pod.clientSecret);
        addParam<>(jo, QStringLiteral("id_server"), pod.idServer);
        addParam<>(jo, QStringLiteral("sid"), pod.sid);
        return jo;
    }
} // namespace QMatrixClient

static const auto Post3PIDsJobName = QStringLiteral("Post3PIDsJob");

Post3PIDsJob::Post3PIDsJob(const ThreePidCredentials& threePidCreds, Omittable<bool> bind)
    : BaseJob(HttpVerb::Post, Post3PIDsJobName,
        basePath % "/account/3pid")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("three_pid_creds"), threePidCreds);
    addParam<IfNotEmpty>(_data, QStringLiteral("bind"), bind);
    setRequestData(_data);
}

static const auto Delete3pidFromAccountJobName = QStringLiteral("Delete3pidFromAccountJob");

Delete3pidFromAccountJob::Delete3pidFromAccountJob(const QString& medium, const QString& address)
    : BaseJob(HttpVerb::Post, Delete3pidFromAccountJobName,
        basePath % "/account/3pid/delete")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("medium"), medium);
    addParam<>(_data, QStringLiteral("address"), address);
    setRequestData(_data);
}

class RequestTokenTo3PIDEmailJob::Private
{
    public:
        Sid data;
};

static const auto RequestTokenTo3PIDEmailJobName = QStringLiteral("RequestTokenTo3PIDEmailJob");

RequestTokenTo3PIDEmailJob::RequestTokenTo3PIDEmailJob(const QString& clientSecret, const QString& email, int sendAttempt, const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post, RequestTokenTo3PIDEmailJobName,
        basePath % "/account/3pid/email/requestToken", false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("email"), email);
    addParam<>(_data, QStringLiteral("send_attempt"), sendAttempt);
    addParam<IfNotEmpty>(_data, QStringLiteral("next_link"), nextLink);
    addParam<>(_data, QStringLiteral("id_server"), idServer);
    setRequestData(_data);
}

RequestTokenTo3PIDEmailJob::~RequestTokenTo3PIDEmailJob() = default;

const Sid& RequestTokenTo3PIDEmailJob::data() const
{
    return d->data;
}

BaseJob::Status RequestTokenTo3PIDEmailJob::parseJson(const QJsonDocument& data)
{
    d->data = fromJson<Sid>(data);
    return Success;
}

class RequestTokenTo3PIDMSISDNJob::Private
{
    public:
        Sid data;
};

static const auto RequestTokenTo3PIDMSISDNJobName = QStringLiteral("RequestTokenTo3PIDMSISDNJob");

RequestTokenTo3PIDMSISDNJob::RequestTokenTo3PIDMSISDNJob(const QString& clientSecret, const QString& country, const QString& phoneNumber, int sendAttempt, const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post, RequestTokenTo3PIDMSISDNJobName,
        basePath % "/account/3pid/msisdn/requestToken", false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("client_secret"), clientSecret);
    addParam<>(_data, QStringLiteral("country"), country);
    addParam<>(_data, QStringLiteral("phone_number"), phoneNumber);
    addParam<>(_data, QStringLiteral("send_attempt"), sendAttempt);
    addParam<IfNotEmpty>(_data, QStringLiteral("next_link"), nextLink);
    addParam<>(_data, QStringLiteral("id_server"), idServer);
    setRequestData(_data);
}

RequestTokenTo3PIDMSISDNJob::~RequestTokenTo3PIDMSISDNJob() = default;

const Sid& RequestTokenTo3PIDMSISDNJob::data() const
{
    return d->data;
}

BaseJob::Status RequestTokenTo3PIDMSISDNJob::parseJson(const QJsonDocument& data)
{
    d->data = fromJson<Sid>(data);
    return Success;
}


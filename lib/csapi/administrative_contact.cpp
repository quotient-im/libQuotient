/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "administrative_contact.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

// Converters
namespace Quotient {

template <>
struct JsonObjectConverter<GetAccount3PIDsJob::ThirdPartyIdentifier> {
    static void fillFrom(const QJsonObject& jo,
                         GetAccount3PIDsJob::ThirdPartyIdentifier& result)
    {
        fromJson(jo.value("medium"_ls), result.medium);
        fromJson(jo.value("address"_ls), result.address);
        fromJson(jo.value("validated_at"_ls), result.validatedAt);
        fromJson(jo.value("added_at"_ls), result.addedAt);
    }
};

} // namespace Quotient

class GetAccount3PIDsJob::Private {
public:
    QVector<ThirdPartyIdentifier> threepids;
};

QUrl GetAccount3PIDsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl),
                                   basePath % "/account/3pid");
}

GetAccount3PIDsJob::GetAccount3PIDsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetAccount3PIDsJob"),
              basePath % "/account/3pid")
    , d(new Private)
{}

GetAccount3PIDsJob::~GetAccount3PIDsJob() = default;

const QVector<GetAccount3PIDsJob::ThirdPartyIdentifier>&
GetAccount3PIDsJob::threepids() const
{
    return d->threepids;
}

BaseJob::Status GetAccount3PIDsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("threepids"_ls), d->threepids);

    return Success;
}

// Converters
namespace Quotient {

template <>
struct JsonObjectConverter<Post3PIDsJob::ThreePidCredentials> {
    static void dumpTo(QJsonObject& jo,
                       const Post3PIDsJob::ThreePidCredentials& pod)
    {
        addParam<>(jo, QStringLiteral("client_secret"), pod.clientSecret);
        addParam<>(jo, QStringLiteral("id_server"), pod.idServer);
        addParam<>(jo, QStringLiteral("sid"), pod.sid);
    }
};

} // namespace Quotient

Post3PIDsJob::Post3PIDsJob(const ThreePidCredentials& threePidCreds,
                           Omittable<bool> bind)
    : BaseJob(HttpVerb::Post, QStringLiteral("Post3PIDsJob"),
              basePath % "/account/3pid")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("three_pid_creds"), threePidCreds);
    addParam<IfNotEmpty>(_data, QStringLiteral("bind"), bind);
    setRequestData(_data);
}

Delete3pidFromAccountJob::Delete3pidFromAccountJob(const QString& medium,
                                                   const QString& address)
    : BaseJob(HttpVerb::Post, QStringLiteral("Delete3pidFromAccountJob"),
              basePath % "/account/3pid/delete")
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("medium"), medium);
    addParam<>(_data, QStringLiteral("address"), address);
    setRequestData(_data);
}

class RequestTokenTo3PIDEmailJob::Private {
public:
    Sid data;
};

RequestTokenTo3PIDEmailJob::RequestTokenTo3PIDEmailJob(
    const QString& clientSecret, const QString& email, int sendAttempt,
    const QString& idServer, const QString& nextLink)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenTo3PIDEmailJob"),
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

const Sid& RequestTokenTo3PIDEmailJob::data() const { return d->data; }

BaseJob::Status RequestTokenTo3PIDEmailJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

class RequestTokenTo3PIDMSISDNJob::Private {
public:
    Sid data;
};

RequestTokenTo3PIDMSISDNJob::RequestTokenTo3PIDMSISDNJob(
    const QString& clientSecret, const QString& country,
    const QString& phoneNumber, int sendAttempt, const QString& idServer,
    const QString& nextLink)
    : BaseJob(HttpVerb::Post, QStringLiteral("RequestTokenTo3PIDMSISDNJob"),
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

const Sid& RequestTokenTo3PIDMSISDNJob::data() const { return d->data; }

BaseJob::Status RequestTokenTo3PIDMSISDNJob::parseJson(const QJsonDocument& data)
{
    fromJson(data, d->data);

    return Success;
}

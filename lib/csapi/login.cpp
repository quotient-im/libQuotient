/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "login.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

// Converters
namespace Quotient {

template <>
struct JsonObjectConverter<GetLoginFlowsJob::LoginFlow> {
    static void fillFrom(const QJsonObject& jo,
                         GetLoginFlowsJob::LoginFlow& result)
    {
        fromJson(jo.value("type"_ls), result.type);
    }
};

} // namespace Quotient

class GetLoginFlowsJob::Private {
public:
    QVector<LoginFlow> flows;
};

QUrl GetLoginFlowsJob::makeRequestUrl(QUrl baseUrl)
{
    return BaseJob::makeRequestUrl(std::move(baseUrl), basePath % "/login");
}

GetLoginFlowsJob::GetLoginFlowsJob()
    : BaseJob(HttpVerb::Get, QStringLiteral("GetLoginFlowsJob"),
              basePath % "/login", false)
    , d(new Private)
{}

GetLoginFlowsJob::~GetLoginFlowsJob() = default;

const QVector<GetLoginFlowsJob::LoginFlow>& GetLoginFlowsJob::flows() const
{
    return d->flows;
}

BaseJob::Status GetLoginFlowsJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("flows"_ls), d->flows);

    return Success;
}

class LoginJob::Private {
public:
    QString userId;
    QString accessToken;
    QString homeServer;
    QString deviceId;
    Omittable<DiscoveryInformation> wellKnown;
};

LoginJob::LoginJob(const QString& type,
                   const Omittable<UserIdentifier>& identifier,
                   const QString& password, const QString& token,
                   const QString& deviceId,
                   const QString& initialDeviceDisplayName, const QString& user,
                   const QString& medium, const QString& address)
    : BaseJob(HttpVerb::Post, QStringLiteral("LoginJob"), basePath % "/login",
              false)
    , d(new Private)
{
    QJsonObject _data;
    addParam<>(_data, QStringLiteral("type"), type);
    addParam<IfNotEmpty>(_data, QStringLiteral("identifier"), identifier);
    addParam<IfNotEmpty>(_data, QStringLiteral("password"), password);
    addParam<IfNotEmpty>(_data, QStringLiteral("token"), token);
    addParam<IfNotEmpty>(_data, QStringLiteral("device_id"), deviceId);
    addParam<IfNotEmpty>(_data, QStringLiteral("initial_device_display_name"),
                         initialDeviceDisplayName);
    addParam<IfNotEmpty>(_data, QStringLiteral("user"), user);
    addParam<IfNotEmpty>(_data, QStringLiteral("medium"), medium);
    addParam<IfNotEmpty>(_data, QStringLiteral("address"), address);
    setRequestData(_data);
}

LoginJob::~LoginJob() = default;

const QString& LoginJob::userId() const { return d->userId; }

const QString& LoginJob::accessToken() const { return d->accessToken; }

const QString& LoginJob::homeServer() const { return d->homeServer; }

const QString& LoginJob::deviceId() const { return d->deviceId; }

const Omittable<DiscoveryInformation>& LoginJob::wellKnown() const
{
    return d->wellKnown;
}

BaseJob::Status LoginJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    fromJson(json.value("user_id"_ls), d->userId);
    fromJson(json.value("access_token"_ls), d->accessToken);
    fromJson(json.value("home_server"_ls), d->homeServer);
    fromJson(json.value("device_id"_ls), d->deviceId);
    fromJson(json.value("well_known"_ls), d->wellKnown);

    return Success;
}

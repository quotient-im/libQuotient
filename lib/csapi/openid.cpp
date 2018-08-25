/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "openid.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace QMatrixClient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class RequestOpenIdTokenJob::Private
{
    public:
        QString accessToken;
        QString tokenType;
        QString matrixServerName;
        int expiresIn;
};

static const auto RequestOpenIdTokenJobName = QStringLiteral("RequestOpenIdTokenJob");

RequestOpenIdTokenJob::RequestOpenIdTokenJob(const QString& userId, const QJsonObject& body)
    : BaseJob(HttpVerb::Post, RequestOpenIdTokenJobName,
        basePath % "/user/" % userId % "/openid/request_token")
    , d(new Private)
{
    setRequestData(Data(toJson(body)));
}

RequestOpenIdTokenJob::~RequestOpenIdTokenJob() = default;

const QString& RequestOpenIdTokenJob::accessToken() const
{
    return d->accessToken;
}

const QString& RequestOpenIdTokenJob::tokenType() const
{
    return d->tokenType;
}

const QString& RequestOpenIdTokenJob::matrixServerName() const
{
    return d->matrixServerName;
}

int RequestOpenIdTokenJob::expiresIn() const
{
    return d->expiresIn;
}

BaseJob::Status RequestOpenIdTokenJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("access_token"_ls))
        return { JsonParseError,
            "The key 'access_token' not found in the response" };
    d->accessToken = fromJson<QString>(json.value("access_token"_ls));
    if (!json.contains("token_type"_ls))
        return { JsonParseError,
            "The key 'token_type' not found in the response" };
    d->tokenType = fromJson<QString>(json.value("token_type"_ls));
    if (!json.contains("matrix_server_name"_ls))
        return { JsonParseError,
            "The key 'matrix_server_name' not found in the response" };
    d->matrixServerName = fromJson<QString>(json.value("matrix_server_name"_ls));
    if (!json.contains("expires_in"_ls))
        return { JsonParseError,
            "The key 'expires_in' not found in the response" };
    d->expiresIn = fromJson<int>(json.value("expires_in"_ls));
    return Success;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "openid.h"

#include "converters.h"

#include <QtCore/QStringBuilder>

using namespace Quotient;

static const auto basePath = QStringLiteral("/_matrix/client/r0");

class RequestOpenIdTokenJob::Private
{
public:
    QString accessToken;
    QString tokenType;
    QString matrixServerName;
    int expiresIn;
};

static const auto RequestOpenIdTokenJobName =
    QStringLiteral("RequestOpenIdTokenJob");

RequestOpenIdTokenJob::RequestOpenIdTokenJob(const QString& userId,
                                             const QJsonObject& body)
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

const QString& RequestOpenIdTokenJob::tokenType() const { return d->tokenType; }

const QString& RequestOpenIdTokenJob::matrixServerName() const
{
    return d->matrixServerName;
}

int RequestOpenIdTokenJob::expiresIn() const { return d->expiresIn; }

BaseJob::Status RequestOpenIdTokenJob::parseJson(const QJsonDocument& data)
{
    auto json = data.object();
    if (!json.contains("access_token"_ls))
        return { IncorrectResponse,
                 "The key 'access_token' not found in the response" };
    fromJson(json.value("access_token"_ls), d->accessToken);
    if (!json.contains("token_type"_ls))
        return { IncorrectResponse,
                 "The key 'token_type' not found in the response" };
    fromJson(json.value("token_type"_ls), d->tokenType);
    if (!json.contains("matrix_server_name"_ls))
        return { IncorrectResponse,
                 "The key 'matrix_server_name' not found in the response" };
    fromJson(json.value("matrix_server_name"_ls), d->matrixServerName);
    if (!json.contains("expires_in"_ls))
        return { IncorrectResponse,
                 "The key 'expires_in' not found in the response" };
    fromJson(json.value("expires_in"_ls), d->expiresIn);

    return Success;
}

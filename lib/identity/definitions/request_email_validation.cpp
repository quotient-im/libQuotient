/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "request_email_validation.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const RequestEmailValidation& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("client_secret"), pod.clientSecret);
    addParam<>(jo, QStringLiteral("email"), pod.email);
    addParam<>(jo, QStringLiteral("send_attempt"), pod.sendAttempt);
    addParam<IfNotEmpty>(jo, QStringLiteral("next_link"), pod.nextLink);
    return jo;
}

RequestEmailValidation FromJsonObject<RequestEmailValidation>::operator()(const QJsonObject& jo) const
{
    RequestEmailValidation result;
    result.clientSecret =
        fromJson<QString>(jo.value("client_secret"_ls));
    result.email =
        fromJson<QString>(jo.value("email"_ls));
    result.sendAttempt =
        fromJson<int>(jo.value("send_attempt"_ls));
    result.nextLink =
        fromJson<QString>(jo.value("next_link"_ls));

    return result;
}


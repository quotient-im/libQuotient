/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "request_msisdn_validation.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const RequestMsisdnValidation& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("client_secret"), pod.clientSecret);
    addParam<>(jo, QStringLiteral("country"), pod.country);
    addParam<>(jo, QStringLiteral("phone_number"), pod.phoneNumber);
    addParam<>(jo, QStringLiteral("send_attempt"), pod.sendAttempt);
    addParam<IfNotEmpty>(jo, QStringLiteral("next_link"), pod.nextLink);
    return jo;
}

RequestMsisdnValidation FromJsonObject<RequestMsisdnValidation>::operator()(const QJsonObject& jo) const
{
    RequestMsisdnValidation result;
    result.clientSecret =
        fromJson<QString>(jo.value("client_secret"_ls));
    result.country =
        fromJson<QString>(jo.value("country"_ls));
    result.phoneNumber =
        fromJson<QString>(jo.value("phone_number"_ls));
    result.sendAttempt =
        fromJson<int>(jo.value("send_attempt"_ls));
    result.nextLink =
        fromJson<QString>(jo.value("next_link"_ls));

    return result;
}


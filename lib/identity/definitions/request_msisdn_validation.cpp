/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "request_msisdn_validation.h"

using namespace QMatrixClient;

void JsonObjectConverter<RequestMsisdnValidation>::dumpTo(
        QJsonObject& jo, const RequestMsisdnValidation& pod)
{
    addParam<>(jo, QStringLiteral("client_secret"), pod.clientSecret);
    addParam<>(jo, QStringLiteral("country"), pod.country);
    addParam<>(jo, QStringLiteral("phone_number"), pod.phoneNumber);
    addParam<>(jo, QStringLiteral("send_attempt"), pod.sendAttempt);
    addParam<IfNotEmpty>(jo, QStringLiteral("next_link"), pod.nextLink);
}

void JsonObjectConverter<RequestMsisdnValidation>::fillFrom(
    const QJsonObject& jo, RequestMsisdnValidation& result)
{
    fromJson(jo.value("client_secret"_ls), result.clientSecret);
    fromJson(jo.value("country"_ls), result.country);
    fromJson(jo.value("phone_number"_ls), result.phoneNumber);
    fromJson(jo.value("send_attempt"_ls), result.sendAttempt);
    fromJson(jo.value("next_link"_ls), result.nextLink);
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "request_email_validation.h"

using namespace Quotient;

void JsonObjectConverter<RequestEmailValidation>::dumpTo(
    QJsonObject& jo, const RequestEmailValidation& pod)
{
    addParam<>(jo, QStringLiteral("client_secret"), pod.clientSecret);
    addParam<>(jo, QStringLiteral("email"), pod.email);
    addParam<>(jo, QStringLiteral("send_attempt"), pod.sendAttempt);
    addParam<IfNotEmpty>(jo, QStringLiteral("next_link"), pod.nextLink);
}

void JsonObjectConverter<RequestEmailValidation>::fillFrom(
    const QJsonObject& jo, RequestEmailValidation& result)
{
    fromJson(jo.value("client_secret"_ls), result.clientSecret);
    fromJson(jo.value("email"_ls), result.email);
    fromJson(jo.value("send_attempt"_ls), result.sendAttempt);
    fromJson(jo.value("next_link"_ls), result.nextLink);
}

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "user_identifier.h"

using namespace QMatrixClient;

void JsonObjectConverter<UserIdentifier>::dumpTo(
        QJsonObject& jo, const UserIdentifier& pod)
{
    fillJson(jo, pod.additionalProperties);
    addParam<>(jo, QStringLiteral("type"), pod.type);
}

void JsonObjectConverter<UserIdentifier>::fillFrom(
    QJsonObject jo, UserIdentifier& result)
{
    fromJson(jo.take("type"_ls), result.type);

    fromJson(jo, result.additionalProperties);
}


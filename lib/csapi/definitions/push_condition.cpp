/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_condition.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PushCondition& pod)
{
    QJsonObject jo;
    addParam<>(jo, QStringLiteral("kind"), pod.kind);
    addParam<IfNotEmpty>(jo, QStringLiteral("key"), pod.key);
    addParam<IfNotEmpty>(jo, QStringLiteral("pattern"), pod.pattern);
    addParam<IfNotEmpty>(jo, QStringLiteral("is"), pod.is);
    return jo;
}

PushCondition FromJsonObject<PushCondition>::operator()(const QJsonObject& jo) const
{
    PushCondition result;
    result.kind =
        fromJson<QString>(jo.value("kind"_ls));
    result.key =
        fromJson<QString>(jo.value("key"_ls));
    result.pattern =
        fromJson<QString>(jo.value("pattern"_ls));
    result.is =
        fromJson<QString>(jo.value("is"_ls));

    return result;
}


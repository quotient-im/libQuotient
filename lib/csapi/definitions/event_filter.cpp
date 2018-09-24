/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "event_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const Filter& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("limit"), pod.limit);
    addParam<IfNotEmpty>(jo, QStringLiteral("not_senders"), pod.notSenders);
    addParam<IfNotEmpty>(jo, QStringLiteral("not_types"), pod.notTypes);
    addParam<IfNotEmpty>(jo, QStringLiteral("senders"), pod.senders);
    addParam<IfNotEmpty>(jo, QStringLiteral("types"), pod.types);
    return jo;
}

Filter FromJsonObject<Filter>::operator()(const QJsonObject& jo) const
{
    Filter result;
    result.limit =
        fromJson<int>(jo.value("limit"_ls));
    result.notSenders =
        fromJson<QStringList>(jo.value("not_senders"_ls));
    result.notTypes =
        fromJson<QStringList>(jo.value("not_types"_ls));
    result.senders =
        fromJson<QStringList>(jo.value("senders"_ls));
    result.types =
        fromJson<QStringList>(jo.value("types"_ls));

    return result;
}


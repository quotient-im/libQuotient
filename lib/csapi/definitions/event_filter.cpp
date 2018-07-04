/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "event_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const Filter& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("limit"), pod.limit);
    addParam<IfNotEmpty>(_json, QStringLiteral("not_senders"), pod.notSenders);
    addParam<IfNotEmpty>(_json, QStringLiteral("not_types"), pod.notTypes);
    addParam<IfNotEmpty>(_json, QStringLiteral("senders"), pod.senders);
    addParam<IfNotEmpty>(_json, QStringLiteral("types"), pod.types);
    return _json;
}

Filter FromJson<Filter>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    Filter result;
    result.limit =
        fromJson<int>(_json.value("limit"_ls));
    result.notSenders =
        fromJson<QStringList>(_json.value("not_senders"_ls));
    result.notTypes =
        fromJson<QStringList>(_json.value("not_types"_ls));
    result.senders =
        fromJson<QStringList>(_json.value("senders"_ls));
    result.types =
        fromJson<QStringList>(_json.value("types"_ls));
    
    return result;
}


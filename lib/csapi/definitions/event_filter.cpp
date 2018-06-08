/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "event_filter.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const Filter& pod)
{
    QJsonObject _json;
    addToJson<IfNotEmpty>(_json, "limit", pod.limit);
    addToJson<IfNotEmpty>(_json, "not_senders", pod.notSenders);
    addToJson<IfNotEmpty>(_json, "not_types", pod.notTypes);
    addToJson<IfNotEmpty>(_json, "senders", pod.senders);
    addToJson<IfNotEmpty>(_json, "types", pod.types);
    return _json;
}

Filter FromJson<Filter>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    Filter result;
    result.limit =
        fromJson<int>(_json.value("limit"));
    result.notSenders =
        fromJson<QStringList>(_json.value("not_senders"));
    result.notTypes =
        fromJson<QStringList>(_json.value("not_types"));
    result.senders =
        fromJson<QStringList>(_json.value("senders"));
    result.types =
        fromJson<QStringList>(_json.value("types"));
    
    return result;
}


/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_ruleset.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PushRuleset& pod)
{
    QJsonObject _json;
    addToJson<IfNotEmpty>(_json, "content", pod.content);
    addToJson<IfNotEmpty>(_json, "override", pod.override);
    addToJson<IfNotEmpty>(_json, "room", pod.room);
    addToJson<IfNotEmpty>(_json, "sender", pod.sender);
    addToJson<IfNotEmpty>(_json, "underride", pod.underride);
    return _json;
}

PushRuleset FromJson<PushRuleset>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    PushRuleset result;
    result.content =
        fromJson<QVector<PushRule>>(_json.value("content"));
    result.override =
        fromJson<QVector<PushRule>>(_json.value("override"));
    result.room =
        fromJson<QVector<PushRule>>(_json.value("room"));
    result.sender =
        fromJson<QVector<PushRule>>(_json.value("sender"));
    result.underride =
        fromJson<QVector<PushRule>>(_json.value("underride"));
    
    return result;
}


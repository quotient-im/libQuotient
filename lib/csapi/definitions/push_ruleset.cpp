/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_ruleset.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PushRuleset& pod)
{
    QJsonObject _json;
    addParam<IfNotEmpty>(_json, QStringLiteral("content"), pod.content);
    addParam<IfNotEmpty>(_json, QStringLiteral("override"), pod.override);
    addParam<IfNotEmpty>(_json, QStringLiteral("room"), pod.room);
    addParam<IfNotEmpty>(_json, QStringLiteral("sender"), pod.sender);
    addParam<IfNotEmpty>(_json, QStringLiteral("underride"), pod.underride);
    return _json;
}

PushRuleset FromJson<PushRuleset>::operator()(const QJsonValue& jv)
{
    const auto& _json = jv.toObject();
    PushRuleset result;
    result.content =
        fromJson<QVector<PushRule>>(_json.value("content"_ls));
    result.override =
        fromJson<QVector<PushRule>>(_json.value("override"_ls));
    result.room =
        fromJson<QVector<PushRule>>(_json.value("room"_ls));
    result.sender =
        fromJson<QVector<PushRule>>(_json.value("sender"_ls));
    result.underride =
        fromJson<QVector<PushRule>>(_json.value("underride"_ls));
    
    return result;
}


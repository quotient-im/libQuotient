/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_ruleset.h"

using namespace QMatrixClient;

QJsonObject QMatrixClient::toJson(const PushRuleset& pod)
{
    QJsonObject jo;
    addParam<IfNotEmpty>(jo, QStringLiteral("content"), pod.content);
    addParam<IfNotEmpty>(jo, QStringLiteral("override"), pod.override);
    addParam<IfNotEmpty>(jo, QStringLiteral("room"), pod.room);
    addParam<IfNotEmpty>(jo, QStringLiteral("sender"), pod.sender);
    addParam<IfNotEmpty>(jo, QStringLiteral("underride"), pod.underride);
    return jo;
}

PushRuleset FromJsonObject<PushRuleset>::operator()(const QJsonObject& jo) const
{
    PushRuleset result;
    result.content =
        fromJson<QVector<PushRule>>(jo.value("content"_ls));
    result.override =
        fromJson<QVector<PushRule>>(jo.value("override"_ls));
    result.room =
        fromJson<QVector<PushRule>>(jo.value("room"_ls));
    result.sender =
        fromJson<QVector<PushRule>>(jo.value("sender"_ls));
    result.underride =
        fromJson<QVector<PushRule>>(jo.value("underride"_ls));

    return result;
}


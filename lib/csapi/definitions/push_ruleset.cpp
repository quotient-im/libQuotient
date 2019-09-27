/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "push_ruleset.h"

using namespace Quotient;

void JsonObjectConverter<PushRuleset>::dumpTo(QJsonObject& jo,
                                              const PushRuleset& pod)
{
    addParam<IfNotEmpty>(jo, QStringLiteral("content"), pod.content);
    addParam<IfNotEmpty>(jo, QStringLiteral("override"), pod.override);
    addParam<IfNotEmpty>(jo, QStringLiteral("room"), pod.room);
    addParam<IfNotEmpty>(jo, QStringLiteral("sender"), pod.sender);
    addParam<IfNotEmpty>(jo, QStringLiteral("underride"), pod.underride);
}

void JsonObjectConverter<PushRuleset>::fillFrom(const QJsonObject& jo,
                                                PushRuleset& result)
{
    fromJson(jo.value("content"_ls), result.content);
    fromJson(jo.value("override"_ls), result.override);
    fromJson(jo.value("room"_ls), result.room);
    fromJson(jo.value("sender"_ls), result.sender);
    fromJson(jo.value("underride"_ls), result.underride);
}

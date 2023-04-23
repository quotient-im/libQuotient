/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/converters.h>
#include <Quotient/csapi/definitions/push_rule.h>

namespace Quotient {

struct PushRuleset {
    QVector<PushRule> content;

    QVector<PushRule> override;

    QVector<PushRule> room;

    QVector<PushRule> sender;

    QVector<PushRule> underride;
};

template <>
struct JsonObjectConverter<PushRuleset> {
    static void dumpTo(QJsonObject& jo, const PushRuleset& pod)
    {
        addParam<IfNotEmpty>(jo, QStringLiteral("content"), pod.content);
        addParam<IfNotEmpty>(jo, QStringLiteral("override"), pod.override);
        addParam<IfNotEmpty>(jo, QStringLiteral("room"), pod.room);
        addParam<IfNotEmpty>(jo, QStringLiteral("sender"), pod.sender);
        addParam<IfNotEmpty>(jo, QStringLiteral("underride"), pod.underride);
    }
    static void fillFrom(const QJsonObject& jo, PushRuleset& pod)
    {
        fromJson(jo.value("content"_ls), pod.content);
        fromJson(jo.value("override"_ls), pod.override);
        fromJson(jo.value("room"_ls), pod.room);
        fromJson(jo.value("sender"_ls), pod.sender);
        fromJson(jo.value("underride"_ls), pod.underride);
    }
};

} // namespace Quotient

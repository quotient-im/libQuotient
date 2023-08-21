/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include <Quotient/csapi/definitions/push_rule.h>

#include <Quotient/converters.h>

namespace Quotient {

struct QUOTIENT_API PushRuleset {
    QVector<PushRule> content{};

    QVector<PushRule> override{};

    QVector<PushRule> room{};

    QVector<PushRule> sender{};

    QVector<PushRule> underride{};
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
        fillFromJson(jo.value("content"_ls), pod.content);
        fillFromJson(jo.value("override"_ls), pod.override);
        fillFromJson(jo.value("room"_ls), pod.room);
        fillFromJson(jo.value("sender"_ls), pod.sender);
        fillFromJson(jo.value("underride"_ls), pod.underride);
    }
};

} // namespace Quotient

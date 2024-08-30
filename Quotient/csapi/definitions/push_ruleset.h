// THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN

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
        addParam<IfNotEmpty>(jo, "content"_L1, pod.content);
        addParam<IfNotEmpty>(jo, "override"_L1, pod.override);
        addParam<IfNotEmpty>(jo, "room"_L1, pod.room);
        addParam<IfNotEmpty>(jo, "sender"_L1, pod.sender);
        addParam<IfNotEmpty>(jo, "underride"_L1, pod.underride);
    }
    static void fillFrom(const QJsonObject& jo, PushRuleset& pod)
    {
        fillFromJson(jo.value("content"_L1), pod.content);
        fillFromJson(jo.value("override"_L1), pod.override);
        fillFromJson(jo.value("room"_L1), pod.room);
        fillFromJson(jo.value("sender"_L1), pod.sender);
        fillFromJson(jo.value("underride"_L1), pod.underride);
    }
};

} // namespace Quotient

/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "converters.h"

namespace Quotient {
/// A stripped down state event, with only the `type`, `state_key`,
/// `sender`, and `content` keys.
struct StrippedStateEvent {
    /// The `content` for the event.
    QJsonObject content;

    /// The `state_key` for the event.
    QString stateKey;

    /// The `type` for the event.
    QString type;

    /// The `sender` for the event.
    QString sender;
};

template <>
struct JsonObjectConverter<StrippedStateEvent> {
    static void dumpTo(QJsonObject& jo, const StrippedStateEvent& pod)
    {
        addParam<>(jo, QStringLiteral("content"), pod.content);
        addParam<>(jo, QStringLiteral("state_key"), pod.stateKey);
        addParam<>(jo, QStringLiteral("type"), pod.type);
        addParam<>(jo, QStringLiteral("sender"), pod.sender);
    }
    static void fillFrom(const QJsonObject& jo, StrippedStateEvent& pod)
    {
        fromJson(jo.value("content"_ls), pod.content);
        fromJson(jo.value("state_key"_ls), pod.stateKey);
        fromJson(jo.value("type"_ls), pod.type);
        fromJson(jo.value("sender"_ls), pod.sender);
    }
};

} // namespace Quotient

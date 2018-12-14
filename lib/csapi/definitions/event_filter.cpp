/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#include "event_filter.h"

using namespace QMatrixClient;

void JsonObjectConverter<EventFilter>::dumpTo(
        QJsonObject& jo, const EventFilter& pod)
{
    addParam<IfNotEmpty>(jo, QStringLiteral("limit"), pod.limit);
    addParam<IfNotEmpty>(jo, QStringLiteral("not_senders"), pod.notSenders);
    addParam<IfNotEmpty>(jo, QStringLiteral("not_types"), pod.notTypes);
    addParam<IfNotEmpty>(jo, QStringLiteral("senders"), pod.senders);
    addParam<IfNotEmpty>(jo, QStringLiteral("types"), pod.types);
}

void JsonObjectConverter<EventFilter>::fillFrom(
    const QJsonObject& jo, EventFilter& result)
{
    fromJson(jo.value("limit"_ls), result.limit);
    fromJson(jo.value("not_senders"_ls), result.notSenders);
    fromJson(jo.value("not_types"_ls), result.notTypes);
    fromJson(jo.value("senders"_ls), result.senders);
    fromJson(jo.value("types"_ls), result.types);
}


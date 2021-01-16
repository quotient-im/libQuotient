// SPDX-FileCopyrightText: 2019 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "reactionevent.h"

using namespace Quotient;

void JsonObjectConverter<EventRelation>::dumpTo(
    QJsonObject& jo, const EventRelation& pod)
{
    if (pod.type.isEmpty()) {
        qCWarning(MAIN) << "Empty relation type; won't dump to JSON";
        return;
    }
    jo.insert(QStringLiteral("rel_type"), pod.type);
    jo.insert(EventIdKey, pod.eventId);
    if (pod.type == EventRelation::Annotation())
        jo.insert(QStringLiteral("key"), pod.key);
}

void JsonObjectConverter<EventRelation>::fillFrom(
    const QJsonObject& jo, EventRelation& pod)
{
    // The experimental logic for generic relationships (MSC1849)
    fromJson(jo["rel_type"_ls], pod.type);
    fromJson(jo[EventIdKeyL], pod.eventId);
    if (pod.type == EventRelation::Annotation())
        fromJson(jo["key"_ls], pod.key);
}

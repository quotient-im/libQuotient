// SPDX-FileCopyrightText: 2022 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventrelation.h"

#include "../logging_categories_p.h"
#include "roomevent.h"

using namespace Quotient;

void JsonObjectConverter<EventRelation>::dumpTo(QJsonObject& jo,
                                                const EventRelation& pod)
{
    if (pod.type.isEmpty()) {
        qCWarning(MAIN) << "Empty relation type; won't dump to JSON";
        return;
    }
    jo.insert(RelTypeKey, pod.type);
    jo.insert(EventIdKey, pod.eventId);
    if (pod.type == EventRelation::AnnotationType)
        jo.insert(QStringLiteral("key"), pod.key);
}

void JsonObjectConverter<EventRelation>::fillFrom(const QJsonObject& jo,
                                                  EventRelation& pod)
{
    if (const auto replyJson = jo.value(EventRelation::ReplyType).toObject();
        !replyJson.isEmpty()) {
        pod.type = EventRelation::ReplyType;
        fromJson(replyJson[EventIdKey], pod.eventId);
    } else {
        // The experimental logic for generic relationships (MSC1849)
        fromJson(jo[RelTypeKey], pod.type);
        fromJson(jo[EventIdKey], pod.eventId);
        if (pod.type == EventRelation::AnnotationType)
            fromJson(jo["key"_L1], pod.key);
    }
}

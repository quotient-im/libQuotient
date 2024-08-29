// SPDX-FileCopyrightText: 2022 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventrelation.h"

#include "../logging_categories_p.h"
#include "converters.h"
#include "roomevent.h"

using namespace Quotient;

void JsonObjectConverter<EventRelation>::dumpTo(QJsonObject& jo,
                                                const EventRelation& pod)
{
    if (pod.type.isEmpty()) {
        qCWarning(MAIN) << "Empty relation type; won't dump to JSON";
        return;
    }

    if (pod.type == EventRelation::ReplyType) {
        jo.insert(RelTypeKey, {{EventIdKey, pod.eventId}});
        return;
    }

    jo.insert(RelTypeKey, pod.type);
    jo.insert(EventIdKey, pod.eventId);
    if (pod.type == EventRelation::AnnotationType)
        jo.insert("key"_L1, pod.key);
    if (pod.type == EventRelation::ThreadType) {
        jo.insert(RelTypeKey, {{EventIdKey, pod.fallbackEventId}});
    }
    jo.insert(IsFallingBackKey, pod.isFallingBack);
}

void JsonObjectConverter<EventRelation>::fillFrom(const QJsonObject& jo,
                                                  EventRelation& pod)
{
    const auto replyJson = jo.value(EventRelation::ReplyType).toObject();
    if (!replyJson.isEmpty() && jo.value(RelTypeKey).isUndefined()) {
        pod.type = EventRelation::ReplyType;
        fromJson(replyJson[EventIdKey], pod.eventId);
        return;
    }

    // The experimental logic for generic relationships (MSC1849)
    fromJson(jo[RelTypeKey], pod.type);
    fromJson(jo[EventIdKey], pod.eventId);
    if (pod.type == EventRelation::AnnotationType)
        fromJson(jo["key"_L1], pod.key);
    if (pod.type == EventRelation::ThreadType) {
        fromJson(replyJson[EventIdKey], pod.fallbackEventId);
    }
    fromJson(jo[IsFallingBackKey], pod.isFallingBack);
}

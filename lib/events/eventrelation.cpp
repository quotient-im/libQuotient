// SPDX-FileCopyrightText: 2022 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "eventrelation.h"

#include "../logging.h"
#include "event.h"

using namespace Quotient;

void JsonObjectConverter<EventRelation>::dumpTo(QJsonObject& jo,
                                                const EventRelation& pod)
{
    if (pod.type.isEmpty()) {
        qCWarning(MAIN) << "Empty relation type; won't dump to JSON";
        return;
    }
    jo.insert(RelTypeKey, pod.type);
    jo.insert(EventIdKeyL, pod.eventId);
    if (pod.type == EventRelation::AnnotationType)
        jo.insert(QStringLiteral("key"), pod.key);
}

void JsonObjectConverter<EventRelation>::fillFrom(const QJsonObject& jo,
                                                  EventRelation& pod)
{
    if (const auto replyJson = jo.value(EventRelation::ReplyType).toObject();
        !replyJson.isEmpty()) {
        pod.type = EventRelation::ReplyType;
        fromJson(replyJson[EventIdKeyL], pod.eventId);
    } else {
        // The experimental logic for generic relationships (MSC1849)
        fromJson(jo[RelTypeKey], pod.type);
        fromJson(jo[EventIdKeyL], pod.eventId);
        if (pod.type == EventRelation::AnnotationType)
            fromJson(jo["key"_ls], pod.key);
    }
}

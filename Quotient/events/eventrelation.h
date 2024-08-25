// SPDX-FileCopyrightText: 2022 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Quotient/converters.h>

namespace Quotient {

constexpr inline auto RelTypeKey = "rel_type"_L1;

struct QUOTIENT_API EventRelation {
    using reltypeid_t = QLatin1String;

    QString type;
    QString eventId;
    QString key = {}; // Only used for m.annotation for now

    static constexpr auto ReplyType = "m.in_reply_to"_L1;
    static constexpr auto AnnotationType = "m.annotation"_L1;
    static constexpr auto ReplacementType = "m.replace"_L1;

    static EventRelation replyTo(QString eventId)
    {
        return { ReplyType, std::move(eventId) };
    }
    static EventRelation annotate(QString eventId, QString key)
    {
        return { AnnotationType, std::move(eventId), std::move(key) };
    }
    static EventRelation replace(QString eventId)
    {
        return { ReplacementType, std::move(eventId) };
    }
};

template <>
struct QUOTIENT_API JsonObjectConverter<EventRelation> {
    static void dumpTo(QJsonObject& jo, const EventRelation& pod);
    static void fillFrom(const QJsonObject& jo, EventRelation& pod);
};

} // namespace Quotient

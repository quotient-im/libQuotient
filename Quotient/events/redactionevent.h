// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class QUOTIENT_API RedactionEvent : public RoomEvent {
public:
    QUO_EVENT(RedactionEvent, "m.room.redaction")

    using RoomEvent::RoomEvent;

    [[deprecated("Use redactedEvents() instead")]]
    QString redactedEvent() const
    {
        return fullJson()["redacts"_ls].toString();
    }
    QStringList redactedEvents() const
    {
        const auto evtIdJson = contentJson()["redacts"_ls];
        if (evtIdJson.isArray())
            return fromJson<QStringList>(evtIdJson); // MSC2244: a list of ids
        if (evtIdJson.isString())
            return { fromJson<QString>(evtIdJson) }; // MSC2174: id in content
        return { fullJson()["redacts"_ls].toString() }; // legacy fallback
    }

    QUO_CONTENT_GETTER(QString, reason)
};
} // namespace Quotient

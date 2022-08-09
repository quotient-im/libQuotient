// SPDX-FileCopyrightText: 2017 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "roomevent.h"

namespace Quotient {
class QUOTIENT_API RedactionEvent : public RoomEvent {
public:
    QUO_EVENT(RedactionEvent, "m.room.redaction")

    explicit RedactionEvent(const QJsonObject& obj) : RoomEvent(typeId(), obj)
    {}

    QString redactedEvent() const
    {
        return fullJson()["redacts"_ls].toString();
    }
    QUO_CONTENT_GETTER(QString, reason)
};
} // namespace Quotient

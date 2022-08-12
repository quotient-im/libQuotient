// SPDX-FileCopyrightText: 2018 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "event.h"

#include <QtCore/QDateTime>
#include <QtCore/QVector>

namespace Quotient {
struct UserTimestamp {
    QString userId;
    QDateTime timestamp;
};
struct ReceiptsForEvent {
    QString evtId;
    QVector<UserTimestamp> receipts;
};
using EventsWithReceipts = QVector<ReceiptsForEvent>;

template <>
QUOTIENT_API EventsWithReceipts fromJson(const QJsonObject& json);
QUOTIENT_API QJsonObject toJson(const EventsWithReceipts& ewrs);

class QUOTIENT_API ReceiptEvent
    : public EventTemplate<ReceiptEvent, Event, EventsWithReceipts> {
public:
    QUO_EVENT(ReceiptEvent, "m.receipt")
    using EventTemplate::EventTemplate;

    [[deprecated("Use content() instead")]]
    EventsWithReceipts eventsWithReceipts() const { return content(); }
};
} // namespace Quotient

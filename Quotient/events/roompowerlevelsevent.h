// SPDX-FileCopyrightText: 2019 Black Hat <bhat@encom.eu.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"

namespace Quotient {
struct QUOTIENT_API PowerLevelsEventContent {
    struct Notifications {
        int room;
    };

    explicit PowerLevelsEventContent(const QJsonObject& json);
    QJsonObject toJson() const;

    int invite;
    int kick;
    int ban;

    int redact;

    QHash<QString, int> events;
    int eventsDefault;
    int stateDefault;

    QHash<QString, int> users;
    int usersDefault;

    Notifications notifications;
};

class QUOTIENT_API RoomPowerLevelsEvent
    : public KeylessStateEventBase<RoomPowerLevelsEvent, PowerLevelsEventContent> {
public:
    QUO_EVENT(RoomPowerLevelsEvent, "m.room.power_levels")

    using KeylessStateEventBase::KeylessStateEventBase;

    int invite() const { return content().invite; }
    int kick() const { return content().kick; }
    int ban() const { return content().ban; }

    int redact() const { return content().redact; }

    QHash<QString, int> events() const { return content().events; }
    int eventsDefault() const { return content().eventsDefault; }
    int stateDefault() const { return content().stateDefault; }

    QHash<QString, int> users() const { return content().users; }
    int usersDefault() const { return content().usersDefault; }

    int roomNotification() const { return content().notifications.room; }

    int powerLevelForEvent(const QString& eventTypeId) const;
    int powerLevelForState(const QString& eventTypeId) const;
    int powerLevelForUser(const QString& userId) const;
};
} // namespace Quotient

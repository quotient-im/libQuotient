// SPDX-FileCopyrightText: 2019 Black Hat <bhat@encom.eu.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "stateevent.h"

namespace Quotient {

struct QUOTIENT_API PowerLevelsEventContent {
    // See https://spec.matrix.org/v1.11/client-server-api/#mroompower_levels for the defaults

    int invite = 0;
    int kick = 50;
    int ban = 50;

    int redact = 50;

    QHash<QString, int> events{};
    int eventsDefault = 0;
    int stateDefault = 50;

    QHash<QString, int> users{};
    int usersDefault = 0;

    struct Notifications {
        int room = 50;
    };
    Notifications notifications{};
};

template <>
struct QUOTIENT_API JsonConverter<PowerLevelsEventContent> {
    static PowerLevelsEventContent load(const QJsonValue& jv);
    static QJsonObject dump(const PowerLevelsEventContent& c);
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

    //! \brief Get the power level for message events of a given type
    //!
    //! \note You normally should not compare power levels returned from this
    //!       and other powerLevelFor*() functions directly; use
    //!       Room::canSendEvents() instead
    int powerLevelForEvent(const QString& eventTypeId) const;

    //! \brief Get the power level for state events of a given type
    //!
    //! \note You normally should not compare power levels returned from this
    //!       and other powerLevelFor*() functions directly; use
    //!       Room::canSetState() instead
    int powerLevelForState(const QString& eventTypeId) const;

    //! \brief Get the power level for a given user
    //!
    //! \note You normally should not directly use power levels returned by this
    //!       and other powerLevelFor*() functions; use Room API instead
    //! \sa Room::canSend, Room::canSendEvents, Room::canSetState,
    //!     Room::effectivePowerLevel
    int powerLevelForUser(const QString& userId) const;

    template <EventClass EvT>
    int powerLevelForEventType() const
    {
        if constexpr (std::is_base_of_v<StateEvent, EvT>) {
            return powerLevelForState(EvT::TypeId);
        } else {
            return powerLevelForEvent(EvT::TypeId);
        }
    }
};

} // namespace Quotient

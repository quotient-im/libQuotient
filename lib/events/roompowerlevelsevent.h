#pragma once

#include "eventcontent.h"
#include "stateevent.h"

namespace Quotient {
class PowerLevelsEventContent : public EventContent::Base {
public:
    struct Notifications {
        int room;
    };

    explicit PowerLevelsEventContent(const QJsonObject& json);

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

protected:
    void fillJson(QJsonObject* o) const override;
};

class RoomPowerLevelsEvent : public StateEvent<PowerLevelsEventContent> {
    Q_GADGET
public:
    DEFINE_EVENT_TYPEID("m.room.power_levels", RoomPowerLevelsEvent)

    explicit RoomPowerLevelsEvent(const QJsonObject& obj)
        : StateEvent(typeId(), obj)
    {}

    int invite() { return content().invite; }
    int kick() { return content().kick; }
    int ban() { return content().ban; }

    int redact() { return content().redact; }

    QHash<QString, int> events() { return content().events; }
    int eventsDefault() { return content().eventsDefault; }
    int stateDefault() { return content().stateDefault; }

    QHash<QString, int> users() { return content().users; }
    int usersDefault() { return content().usersDefault; }

    int roomNotification() { return content().notifications.room; }

    int powerLevelForEvent(const QString& eventId);
    int powerLevelForUser(const QString& userId);

private:
};

template <>
class EventFactory<RoomPowerLevelsEvent> {
public:
    static event_ptr_tt<RoomPowerLevelsEvent> make(const QJsonObject& json,
                                              const QString&)
    {
        return makeEvent<RoomPowerLevelsEvent>(json);
    }
};

REGISTER_EVENT_TYPE(RoomPowerLevelsEvent)
} // namespace Quotient

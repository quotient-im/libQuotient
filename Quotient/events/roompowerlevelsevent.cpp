// SPDX-FileCopyrightText: 2019 Black Hat <bhat@encom.eu.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roompowerlevelsevent.h"

using namespace Quotient;

PowerLevelsEventContent JsonConverter<PowerLevelsEventContent>::load(const QJsonValue& jv)
{
    const auto& jo = jv.toObject();
    PowerLevelsEventContent c;
#define FROM_JSON(member) fromJson(jo[toSnakeCase(#member##_L1)], c.member)
    FROM_JSON(invite);
    FROM_JSON(kick);
    FROM_JSON(ban);
    FROM_JSON(redact);
    FROM_JSON(events);
    FROM_JSON(eventsDefault);
    FROM_JSON(stateDefault);
    FROM_JSON(users);
    FROM_JSON(usersDefault);
    fromJson(jo["notifications"_L1]["room"_L1], c.notifications.room);
#undef FROM_JSON
    return c;
}

QJsonObject JsonConverter<PowerLevelsEventContent>::dump(const PowerLevelsEventContent& c) {
    return QJsonObject{ { u"invite"_s, c.invite },
                        { u"kick"_s, c.kick },
                        { u"ban"_s, c.ban },
                        { u"redact"_s, c.redact },
                        { u"events"_s, Quotient::toJson(c.events) },
                        { u"events_default"_s, c.eventsDefault },
                        { u"state_default"_s, c.stateDefault },
                        { u"users"_s, Quotient::toJson(c.users) },
                        { u"users_default"_s, c.usersDefault },
                        { u"notifications"_s, QJsonObject{ { u"room"_s, c.notifications.room } } } };
}

int RoomPowerLevelsEvent::powerLevelForEvent(const QString& eventTypeId) const
{
    return events().value(eventTypeId, eventsDefault());
}

int RoomPowerLevelsEvent::powerLevelForState(const QString& eventTypeId) const
{
    return events().value(eventTypeId, stateDefault());
}

int RoomPowerLevelsEvent::powerLevelForUser(const QString& userId) const
{
    return users().value(userId, usersDefault());
}

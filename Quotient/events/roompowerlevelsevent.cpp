// SPDX-FileCopyrightText: 2019 Black Hat <bhat@encom.eu.org>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "roompowerlevelsevent.h"

using namespace Quotient;

// The default values used below are defined in
// https://spec.matrix.org/v1.3/client-server-api/#mroompower_levels
PowerLevelsEventContent::PowerLevelsEventContent(const QJsonObject& json) :
    invite(json["invite"_L1].toInt(50)),
    kick(json["kick"_L1].toInt(50)),
    ban(json["ban"_L1].toInt(50)),
    redact(json["redact"_L1].toInt(50)),
    events(fromJson<QHash<QString, int>>(json["events"_L1])),
    eventsDefault(json["events_default"_L1].toInt(0)),
    stateDefault(json["state_default"_L1].toInt(50)),
    users(fromJson<QHash<QString, int>>(json["users"_L1])),
    usersDefault(json["users_default"_L1].toInt(0)),
    notifications(Notifications{json["notifications"_L1].toObject()["room"_L1].toInt(50)})
{}

QJsonObject PowerLevelsEventContent::toJson() const
{
    return QJsonObject{ { u"invite"_s, invite },
                        { u"kick"_s, kick },
                        { u"ban"_s, ban },
                        { u"redact"_s, redact },
                        { u"events"_s, Quotient::toJson(events) },
                        { u"events_default"_s, eventsDefault },
                        { u"state_default"_s, stateDefault },
                        { u"users"_s, Quotient::toJson(users) },
                        { u"users_default"_s, usersDefault },
                        { u"notifications"_s, QJsonObject{ { u"room"_s, notifications.room } } } };
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
